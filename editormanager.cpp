#include "editormanager.h"
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QFile>
#include <QPointer>
#include <QEventLoop>
#include <QFile>
#include <QWebEngineView>
#include <QTextStream>
#include <QMessageBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QSettings>
#include <QScrollBar>
#include <QTimer>
#include <QRegularExpression>

/**
 * @brief 将 Markdown 文本转换为 HTML，同时提取目录信息
 * @param markdown 输入的 Markdown 原始文本
 * @param outHtml  输出的 HTML 字符串
 * @param outToc   输出的目录项列表
 * @return 转换是否成功
 */
static bool convertMarkdownToHtml(const QString &markdown, QString &outHtml, QList<TocItem> &outToc)
{
    outHtml.clear();
    outToc.clear();
    QStringList lines = markdown.split('\n');   // 按行分割
    QStringList htmlLines;
    bool inCodeBlock = false;                   // 是否在代码块内
    int headingCounter = 0;                    // 用于生成唯一的锚点 ID

    for (const QString &line : lines) {
        // 处理代码块边界
        if (line.startsWith("```")) {
            if (!inCodeBlock) {
                htmlLines.append("<pre><code>");
                inCodeBlock = true;
            } else {
                htmlLines.append("</code></pre>");
                inCodeBlock = false;
            }
            continue;
        }
        // 代码块内的内容：直接转义后原样输出
        if (inCodeBlock) {
            htmlLines.append(line.toHtmlEscaped());
            continue;
        }

        // 处理 Markdown 标题：以 # 开头
        QRegularExpression headingRegex("^(#{1,6})\\s+(.*)$");
        QRegularExpressionMatch match = headingRegex.match(line);
        if (match.hasMatch()) {
            int level = match.captured(1).length();
            QString text = match.captured(2).trimmed();
            QString anchorId = QString("heading-%1").arg(++headingCounter);
            outToc.append({level, text, anchorId});
            htmlLines.append(QString("<h%1 id=\"%2\">%3</h%1>")
                                 .arg(level).arg(anchorId).arg(text.toHtmlEscaped()));
            continue;
        }

        // 处理行内格式：粗体、斜体、行内代码
        QString processed = line;
        processed.replace(QRegularExpression("\\*\\*(.*?)\\*\\*"), "<strong>\\1</strong>");
        processed.replace(QRegularExpression("\\*([^*]+?)\\*"), "<em>\\1</em>");
        processed.replace(QRegularExpression("`(.*?)`"), "<code>\\1</code>");

        // 空行用 <br> 表示，非空行直接加入
        if (!processed.isEmpty()) {
            htmlLines.append(processed);
        } else {
            htmlLines.append("<br>");
        }
    }

    // 组合成完整的 HTML 文档
    outHtml = QString("<html><body>%1</body></html>").arg(htmlLines.join("\n"));
    return true;
}

// 构造函数：初始化标签控件、定时器，连接信号
EditorManager::EditorManager(QWidget *parent)
    : QTabWidget{parent}
    , m_editorStyleSheet(R"(
        QPlainTextEdit {
            font-family: "Courier New", "Consolas", monospace;
            font-size: 13px;
            line-height: 1.5;
            background-color: #ffffff;
            color: #333333;
            border: none;
            padding: 10px;
        }
    )")
{
    setTabsClosable(true);  // 每个标签页显示关闭按钮
    // 当用户点击标签关闭按钮时，调用 closeTab
    connect(this, &EditorManager::tabCloseRequested, this, &EditorManager::closeTab);
    // 当当前标签改变时，调用 onCurrentTabChanged 更新预览
    connect(this, &QTabWidget::currentChanged, this, &EditorManager::onCurrentTabChanged);

    // 防抖定时器：用户停止输入后 200ms 才触发预览生成
    m_previewTimer = new QTimer(this);
    m_previewTimer->setSingleShot(true);
    m_previewTimer->setInterval(200);
    connect(m_previewTimer, &QTimer::timeout, this, &EditorManager::generatePreview);
}

// 新建空白文档
bool EditorManager::newFile()
{

    QPlainTextEdit *edit = new QPlainTextEdit(this);
    // 在 QPlainTextEdit 设置统一的样式表
    if (!m_editorStyleSheet.isEmpty()) {
        edit->setStyleSheet(m_editorStyleSheet);
    }

    QFont font;
    font.setFamily("Courier New");
    font.setPointSize(10);
    edit->setFont(font);

    m_filePathMap[edit] = QString();   // 文件路径为空
    int index = addTab(edit, "Untitled*");
    setCurrentIndex(index);

    TabData data;
    data.editor = edit;
    data.filePath = QString();
    data.isModified = true;
    m_tabData.insert(index, data);

    connect(edit, &QPlainTextEdit::textChanged, this, &EditorManager::onTextChanged);
    schedulePreview();   // 触发预览
    return true;
}
void EditorManager::updateEditorStyle(const QString &styleSheet)
{
    m_editorStyleSheet = styleSheet;
    for (const TabData &data : m_tabData) {
        if (data.editor) {
            data.editor->setStyleSheet(styleSheet);
        }
    }
}

// 打开已有文件
bool EditorManager::openFile(const QString &path)
{
    QPlainTextEdit *edit = new QPlainTextEdit(this);
    // 应用当前主题的编辑器样式
    if (!m_editorStyleSheet.isEmpty()) {
        edit->setStyleSheet(m_editorStyleSheet);
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();

    QFont font;
    font.setFamily("Courier New");
    font.setPointSize(10);
    edit->setFont(font);
    edit->setPlainText(content);

    m_filePathMap[edit] = path;
    QString fileName = QFileInfo(path).fileName();
    int index = addTab(edit, fileName);
    setCurrentIndex(index);

    TabData data;
    data.editor = edit;
    data.filePath = path;
    data.isModified = false;
    m_tabData.insert(index, data);

    connect(edit, &QPlainTextEdit::textChanged, this, &EditorManager::onTextChanged);
    schedulePreview();
    return true;
}

// 编辑器内容改变时的槽函数：更新修改状态和标题，并触发预览
void EditorManager::onTextChanged()
{
    QPlainTextEdit *edit = qobject_cast<QPlainTextEdit*>(sender());
    if (!edit) return;

    for (int i = 0; i < m_tabData.size(); ++i) {
        if (m_tabData[i].editor == edit) {
            if (!m_tabData[i].isModified) {
                m_tabData[i].isModified = true;
                setTabTitle(i);
            }
            break;
        }
    }
    schedulePreview();
}

// 设置指定索引标签的标题（根据路径和修改状态加上星号）
void EditorManager::setTabTitle(int index)
{
    if (index < 0 || index >= m_tabData.size()) return;
    const TabData &data = m_tabData[index];
    QString baseName = data.filePath.isEmpty() ? "Untitled" : QFileInfo(data.filePath).fileName();
    if (data.isModified) baseName += "*";
    setTabText(index, baseName);
}

void EditorManager::updateTabModified(int index, bool modified)
{
    if (index < 0 || index >= m_tabData.size()) return;
    m_tabData[index].isModified = modified;
    setTabTitle(index);
}

// 保存当前文件
bool EditorManager::saveCurrentFile()
{
    int index = currentIndex();
    if (index < 0 || index >= m_tabData.size()) return false;
    TabData &data = m_tabData[index];
    if (data.filePath.isEmpty())
        return saveAs();   // 新建文件转调另存为

    QFile file(data.filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream stream(&file);
    stream << data.editor->toPlainText();
    file.close();

    data.isModified = false;
    setTabTitle(index);
    return true;
}

// 另存为 makedown 或者 PDF
bool EditorManager::saveAs()
{
    int index = currentIndex();
    if (index < 0 || index >= m_tabData.size())
        return false;

    TabData &data = m_tabData[index];

    // 确定默认目录和默认文件名
    QString defaultDir;
    QString defaultFileName;
    if (!data.filePath.isEmpty()) {
        QFileInfo info(data.filePath);
        defaultDir = info.absolutePath();
        defaultFileName = info.fileName();   // 例如 "my_note.md"
    } else {
        defaultFileName = tr("Untitled.md");
        // defaultDir 留空，QFileDialog 会使用上次使用的目录或当前工作目录
    }
    // 构造完整默认路径
    QString defaultPath = defaultDir.isEmpty() ? defaultFileName : defaultDir + "/" + defaultFileName;

    // 文件过滤器
    QString filter = tr("Markdown Files (*.md *.markdown);;PDF Files (*.pdf)");
    QString newPath = QFileDialog::getSaveFileName(this, tr("Save As"),
                                                   defaultPath, filter);
    if (newPath.isEmpty())
        return false;

    QFileInfo fileInfo(newPath);
    QString suffix = fileInfo.suffix().toLower();
    bool isPdf = (suffix == "pdf");

    if (isPdf) {
        return exportToPdf(newPath);
    } else {
        // 保存为 Markdown 文件
        data.filePath = newPath;
        if (!saveCurrentFile()) {
            data.filePath.clear();
            return false;
        }
        return true;
    }
}

// 导出为PDF函数
bool EditorManager::exportToPdf(const QString &filePath)
{
    // 获取当前编辑器内容
    QPlainTextEdit *edit = currentEditor();
    if (!edit || m_tabData.isEmpty())
        return false;

    QString markdown = edit->toPlainText();
    QList<TocItem> toc;
    QString html;
    if (!convertMarkdownToHtml(markdown, html, toc))
        return false;

    // 嵌入当前主题的预览 CSS
    if (!m_previewCss.isEmpty()) {
        html.replace("<html>", QString("<html><head><style>%1</style></head>").arg(m_previewCss));
    }

    // 使用临时 QWebEngineView 渲染 HTML 并导出 PDF
    QWebEngineView view;
    view.setHtml(html);

    // 等待页面加载完成（超时 5 秒）
    QEventLoop loadLoop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loadLoop, &QEventLoop::quit);
    connect(&view, &QWebEngineView::loadFinished, &loadLoop, &QEventLoop::quit);
    timer.start(5000);
    loadLoop.exec();

    if (!view.page()) {
        qWarning() << "Failed to load HTML for PDF export";
        return false;
    }

    // 异步生成 PDF，等待完成
    QEventLoop pdfLoop;
    QPointer<QEventLoop> loopPtr = &pdfLoop;
    bool success = false;

    view.page()->printToPdf([&](const QByteArray &pdfData) {
        if (!pdfData.isEmpty()) {
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(pdfData);
                file.close();
                success = true;
            }
        }
        if (loopPtr)
            loopPtr->quit();
    });

    // 设置超时（10 秒）
    QTimer::singleShot(10000, &pdfLoop, &QEventLoop::quit);
    pdfLoop.exec();

    return success;
}

// 关闭前询问保存
bool EditorManager::maybeSave(int index)
{
    if (!m_tabData[index].isModified) return true;
    QMessageBox::StandardButton ret = QMessageBox::question(this,
                                                            tr("Save Changes"),
                                                            tr("The document has been modified.\nDo you want to save your changes?"),
                                                            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save) {
        setCurrentIndex(index);
        return saveCurrentFile();
    } else if (ret == QMessageBox::Discard) {
        return true;
    } else {
        return false;
    }
}

// 关闭标签
void EditorManager::closeTab(int index)
{
    if (index < 0 || index >= m_tabData.size()) return;
    if (!maybeSave(index)) return;

    QPlainTextEdit *edit = m_tabData[index].editor;
    removeTab(index);     // 从界面上移除标签页，但不会删除 edit
    delete edit;          // 立即删除，而不是 deleteLater
    m_tabData.remove(index);
}

// 保存会话（所有打开的文件路径、光标位置、滚动条位置）
void EditorManager::saveSession()
{
    QSettings settings;
    settings.beginWriteArray("editorSession");
    int validIndex = 0;
    for (int i = 0; i < m_tabData.size(); ++i) {
        const TabData &data = m_tabData[i];
        if (data.filePath.isEmpty()) continue;   // 未保存的新建文档不保存
        settings.setArrayIndex(validIndex);
        settings.setValue("filePath", data.filePath);
        int cursorPos = data.editor->textCursor().position();
        settings.setValue("cursorPos", cursorPos);
        int scrollPos = data.editor->verticalScrollBar()->value();
        settings.setValue("scrollPos", scrollPos);
        validIndex++;
    }
    settings.endArray();

    // 记录当前活动标签在已保存标签列表中的位置
    int current = currentIndex();
    int savedCurrentIndex = -1;
    int curValid = 0;
    for (int i = 0; i < m_tabData.size(); ++i) {
        if (m_tabData[i].filePath.isEmpty()) continue;
        if (i == current) {
            savedCurrentIndex = curValid;
            break;
        }
        curValid++;
    }
    settings.setValue("currentIndex", savedCurrentIndex);
}

// 恢复会话
void EditorManager::restoreSession()
{
    QSettings settings;
    int size = settings.beginReadArray("editorSession");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QString path = settings.value("filePath").toString();
        if (!path.isEmpty() && QFile::exists(path)) {
            openFile(path);   // 打开文件会添加一个新标签
            int index = m_tabData.size() - 1;   // 刚打开的标签在最后
            int cursorPos = settings.value("cursorPos").toInt();
            int scrollPos = settings.value("scrollPos").toInt();
            QTextCursor cursor = m_tabData[index].editor->textCursor();
            cursor.setPosition(cursorPos);
            m_tabData[index].editor->setTextCursor(cursor);
            m_tabData[index].editor->verticalScrollBar()->setValue(scrollPos);
            m_tabData[index].isModified = false;
            setTabTitle(index);
        }
    }
    settings.endArray();
    int saveIndex = settings.value("currentIndex", -1).toInt();
    if (saveIndex >= 0 && saveIndex < count())
        setCurrentIndex(saveIndex);
}

// 防抖：每次内容改变时重新启动定时器
void EditorManager::schedulePreview()
{
    if (m_previewTimer->isActive())
        m_previewTimer->stop();
    m_previewTimer->start();
}

void EditorManager::setPreviewCss(const QString &css)
{
    m_previewCss = css;
}
// 生成预览：获取当前编辑器内容，转换并发射信号
void EditorManager::generatePreview()
{
    QPlainTextEdit *edit = currentEditor();
    if (!edit || m_tabData.isEmpty()) return;

    QString markdown = edit->toPlainText();
    QList<TocItem> toc;
    QString html;
    if (convertMarkdownToHtml(markdown, html, toc)) {
        if (!m_previewCss.isEmpty()) {
            html.replace("<html>", QString("<html><head><style>%1</style></head>").arg(m_previewCss));
        }
        emit previewHtmlReady(html);
        emit tocReady(toc);
    }
}

// 当用户切换到不同标签时，更新预览
void EditorManager::onCurrentTabChanged(int index)
{
    if (index >= 0 && index < m_tabData.size())
        schedulePreview();
}

// 获取当前活动标签中的编辑器
QPlainTextEdit* EditorManager::currentEditor() const
{
    return qobject_cast<QPlainTextEdit*>(currentWidget());
}

// 析构函数
EditorManager::~EditorManager()
{
    // 停止定时器
    if (m_previewTimer) {
        m_previewTimer->stop();
    }
    // 断开所有信号
    disconnect();

    // 注意：不要手动删除 m_tabData 中的 editor，因为 QTabWidget 会自动删除它们
    // 但是我们需要清空容器以避免后续误用
    m_tabData.clear();
    m_filePathMap.clear();
}