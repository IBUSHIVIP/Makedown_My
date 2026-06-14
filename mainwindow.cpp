/**
 * @file        mainwindow.cpp
 * @brief       主窗口实现文件
 * @details     布局搭建、组件初始化、信号槽绑定、主题加载、目录渲染、JS锚点跳转、会话保存
 * @author      我没有会员
 * @date        2026-06-14
 * @version     V1.0
 */
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "EditorManager.h"
#include "thememanager.h"
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QWebEngineView>
#include <QVBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QAction>
#include <QSettings>
#include <QFile>
#include <QCloseEvent>

/**
 * @brief       主窗口构造函数
 * @note        完成布局、控件创建、样式初始化、信号槽绑定、历史主题&会话恢复
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_fileToolBar(nullptr)
    , m_editToolBar(nullptr)
    , m_themeToolBar(nullptr)
    , m_viewToolBar(nullptr)
    , m_currentToolBar(nullptr)
{
    ui->setupUi(this);
    bool exists = QFile::exists(":/icon/5n9dl-zunro-001.ico");
    qDebug() << "Icon exists in resources:" << exists;
    setWindowIcon(QIcon(":/icon/resource/5n9dl-zunro-001.ico"));

    // 主窗口全局样式
    setStyleSheet(R"(
        QMainWindow {
            background-color: #f5f7fa;
        }
        QMenuBar {
            background-color: #f5f7fa;
            border-bottom: 1px solid #e2e8f0;
            font-family: "Microsoft YaHei";
            font-size: 13px;
            padding: 4px;
        }
        QMenuBar::item {
            background-color: transparent;
            padding: 6px 16px;
            border-radius: 6px;
        }
        QMenuBar::item:selected {
            background-color: #e2e8f0;
        }
        QToolBar {
            background-color: #ffffff;
            border: none;
            border-bottom: 1px solid #e2e8f0;
            padding: 6px 12px;
            spacing: 12px;
            min-height: 40px;
        }
        QToolButton {
            background-color: transparent;
            border: none;
            border-radius: 6px;
            padding: 6px 12px;
            font-size: 12px;
        }
        QToolButton:hover {
            background-color: #e2e8f0;
        }
        QStatusBar {
            background-color: #f5f7fa;
            color: #718096;
            font-size: 11px;
            border-top: 1px solid #e2e8f0;
        }
    )");

    // 核心组件初始化
    m_editorManager = new EditorManager(this);
    m_preView = new QWebEngineView(this);
    m_preView->setHtml(R"(
        <html>
            <body style='background:#f8f9fa; padding:20px; font-family:"Microsoft YaHei";'>
                <div style='text-align:center; color:#4299e1;'>
                    <h1>✅ 预览区域正常工作</h1>
                    <p>输入 Markdown 内容后，这里会显示渲染效果。</p>
                </div>
            </body>
        </html>
    )");

    // 左侧目录树初始化
    m_tocTree = new QTreeWidget(this);
    m_tocTree->setHeaderHidden(true);
    m_tocTree->setIndentation(16);
    m_tocTree->setStyleSheet(R"(
        QTreeWidget {
            background-color: transparent;
            border: none;
            font-family: "Microsoft YaHei";
            font-size: 12px;
            outline: none;
        }
        QTreeWidget::item {
            padding: 5px 8px;
            margin: 1px 0;
            border-radius: 6px;
            color: #4a5568;
        }
        QTreeWidget::item:hover {
            background-color: #edf2f7;
        }
        QTreeWidget::item:selected {
            background-color: #e2f0ff;
            color: #2c5282;
        }
    )");

    // 目录树外层容器布局
    QWidget *tocContainer = new QWidget(this);
    QVBoxLayout *tocLayout = new QVBoxLayout(tocContainer);
    tocLayout->setContentsMargins(8, 12, 8, 8);
    tocLayout->setSpacing(8);
    QLabel *tocTitle = new QLabel(tr("📄 目录"), this);
    tocTitle->setStyleSheet(R"(
        font-size: 13px;
        font-weight: 500;
        color: #4a5568;
        padding: 4px 8px;
        background-color: #edf2f7;
        border-radius: 8px;
    )");
    tocLayout->addWidget(tocTitle);
    tocLayout->addWidget(m_tocTree);
    tocContainer->setLayout(tocLayout);
    tocContainer->setStyleSheet(R"(
        QWidget {
            background-color: #f8f9fa;
            border-right: 1px solid #e2e8f0;
        }
    )");
    tocContainer->setMinimumWidth(200);
    tocContainer->setMaximumWidth(280);

    // 整体三分栏分割器布局
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->setHandleWidth(1);
    mainSplitter->setStyleSheet("QSplitter::handle { background-color: #e2e8f0; }");
    mainSplitter->addWidget(tocContainer);

    QSplitter *rightSplitter = new QSplitter(Qt::Horizontal, this);
    rightSplitter->setHandleWidth(1);
    rightSplitter->setStyleSheet("QSplitter::handle { background-color: #e2e8f0; }");
    rightSplitter->addWidget(m_editorManager);
    rightSplitter->addWidget(m_preView);
    rightSplitter->setSizes({this->width() / 2, this->width() / 2});

    mainSplitter->addWidget(rightSplitter);
    mainSplitter->setSizes({220, this->width() - 220});
    setCentralWidget(mainSplitter);

    // 创建动作、菜单、工具栏
    createActions();
    createToolBars();
    createMenuBar();

    // 信号槽绑定：文件操作
    connect(m_newAction, &QAction::triggered, m_editorManager, &EditorManager::newFile);
    connect(m_openAction, &QAction::triggered, this, &MainWindow::onOpenAction);
    connect(m_saveAction, &QAction::triggered, m_editorManager, &EditorManager::saveCurrentFile);
    connect(m_saveAsAction, &QAction::triggered, m_editorManager, &EditorManager::saveAs);

    // 信号槽绑定：主题切换
    connect(m_lightAction, &QAction::triggered, this, [this]() { applyTheme("light"); });
    connect(m_darkAction, &QAction::triggered, this, [this]() { applyTheme("dark"); });
    connect(m_freshAction, &QAction::triggered, this, [this]() { applyTheme("fresh"); });
    connect(m_pinkAction, &QAction::triggered, this, [this]() { applyTheme("pink"); });

    // 信号槽绑定：预览与目录
    connect(m_editorManager, &EditorManager::previewHtmlReady,
            this, [this](const QString &html) {
                if (!html.isEmpty()) {
                    m_preView->setHtml(html);
                }
            });
    connect(m_editorManager, &EditorManager::tocReady, this, &MainWindow::updateToc);
    connect(m_tocTree, &QTreeWidget::itemClicked, this, &MainWindow::onTocItemClicked);

    statusBar()->showMessage(tr("✨ 欢迎使用 Markdown 小清新编辑器"), 10000);

    // 恢复上次使用主题
    QSettings settings;
    QString lastTheme = settings.value("currentTheme").toString();
    if (lastTheme.isEmpty()) {
        lastTheme = "light";
    }
    applyTheme(lastTheme);

    // 恢复上次会话
    m_editorManager->restoreSession();
}

/**
 * @brief       创建所有菜单/工具栏动作对象
 */
void MainWindow::createActions()
{
    // 文件操作
    m_newAction = new QAction(tr("📄 新建"), this);
    m_openAction = new QAction(tr("📂 打开"), this);
    m_saveAction = new QAction(tr("💾 保存"), this);
    m_saveAsAction = new QAction(tr("📁 另存为"), this);

    // 编辑操作（预留接口）
    m_undoAction = new QAction(tr("↩️ 撤销"), this);
    m_redoAction = new QAction(tr("↪️ 重做"), this);
    m_findAction = new QAction(tr("🔍 查找"), this);
    m_replaceAction = new QAction(tr("📝 替换"), this);

    // 主题切换
    m_lightAction = new QAction(tr("☀️ 浅色"), this);
    m_darkAction = new QAction(tr("🌙 深色"), this);
    m_freshAction = new QAction(tr("🍃 小清新"), this);
    m_pinkAction = new QAction(tr("🌸 粉嫩撞色"), this);

    // 视图操作（预留接口）
    m_zoomInAction = new QAction(tr("🔍+ 放大"), this);
    m_zoomOutAction = new QAction(tr("🔍- 缩小"), this);
    m_resetZoomAction = new QAction(tr("⟳ 重置"), this);

    // 禁用未完成功能
    m_undoAction->setEnabled(false);
    m_redoAction->setEnabled(false);
    m_findAction->setEnabled(false);
    m_replaceAction->setEnabled(false);
    m_zoomInAction->setEnabled(false);
    m_zoomOutAction->setEnabled(false);
    m_resetZoomAction->setEnabled(false);
}

/**
 * @brief       加载并应用指定主题
 * @param       themeName  主题标识
 * @note        依次加载主窗口样式、编辑器样式、预览CSS，并持久化主题选择
 */
void MainWindow::applyTheme(const QString &themeName)
{
    if(themeName.isEmpty()) return;

    QString mainStyle = ThemeManager::loadMainStyle(themeName);
    if (!mainStyle.isEmpty()) {
        this->setStyleSheet(mainStyle);
    }

    QString editorStyle = ThemeManager::loadEditorStyle(themeName);
    if (m_editorManager && !editorStyle.isEmpty()) {
        m_editorManager->updateEditorStyle(editorStyle);
    }

    QString previewCss = ThemeManager::loadPreviewCss(themeName);
    if (m_editorManager && !previewCss.isEmpty()) {
        m_editorManager->setPreviewCss(previewCss);
        m_editorManager->refreshPreview();
    }

    // 保存当前主题到本地配置
    QSettings settings;
    settings.setValue("currentTheme", themeName);
    settings.sync();
}

/**
 * @brief       创建顶部菜单栏
 */
void MainWindow::createMenuBar()
{
    QMenuBar *menuBar = this->menuBar();
    menuBar->clear();

    QAction *fileAction = new QAction(tr("文件"), this);
    QAction *editAction = new QAction(tr("编辑"), this);
    QAction *themeAction = new QAction(tr("主题"), this);
    QAction *viewAction = new QAction(tr("视图"), this);
    QAction *helpAction = new QAction(tr("帮助"), this);

    menuBar->addAction(fileAction);
    menuBar->addAction(editAction);
    menuBar->addAction(themeAction);
    menuBar->addAction(viewAction);
    menuBar->addAction(helpAction);

    connect(fileAction, &QAction::triggered, this, &MainWindow::showFileToolBar);
    connect(editAction, &QAction::triggered, this, &MainWindow::showEditToolBar);
    connect(themeAction, &QAction::triggered, this, &MainWindow::showThemeToolBar);
    connect(viewAction, &QAction::triggered, this, &MainWindow::showViewToolBar);
    connect(helpAction, &QAction::triggered, this, [this]() {
        statusBar()->showMessage(tr("帮助功能开发中..."), 1500);
    });
}

/**
 * @brief       创建所有功能工具栏
 */
void MainWindow::createToolBars()
{
    m_defaultToolBar = new QToolBar(this);
    m_defaultToolBar->setObjectName("DefaultToolBar");
    QLabel *welcomeLabel = new QLabel(tr("✨ 欢迎进入老王 Makedown"), this);
    welcomeLabel->setStyleSheet("color: #a0aec0; font-size: 12px; padding: 0 12px;");
    m_defaultToolBar->addWidget(welcomeLabel);

    // 文件工具栏
    m_fileToolBar = new QToolBar(this);
    m_fileToolBar->setObjectName("FileToolBar");
    m_fileToolBar->addAction(m_newAction);
    m_fileToolBar->addAction(m_openAction);
    m_fileToolBar->addAction(m_saveAction);
    m_fileToolBar->addAction(m_saveAsAction);
    m_fileToolBar->addSeparator();
    QLabel *tipLabel = new QLabel(tr("✨ 文件操作"), this);
    tipLabel->setStyleSheet("color: #a0aec0; font-size: 11px; padding: 0 8px;");
    m_fileToolBar->addWidget(tipLabel);

    // 编辑工具栏
    m_editToolBar = new QToolBar(this);
    m_editToolBar->setObjectName("EditToolBar");
    m_editToolBar->addAction(m_undoAction);
    m_editToolBar->addAction(m_redoAction);
    m_editToolBar->addSeparator();
    m_editToolBar->addAction(m_findAction);
    m_editToolBar->addAction(m_replaceAction);

    // 主题工具栏
    m_themeToolBar = new QToolBar(this);
    m_themeToolBar->setObjectName("ThemeToolBar");
    m_themeToolBar->addAction(m_lightAction);
    m_themeToolBar->addAction(m_darkAction);
    m_themeToolBar->addAction(m_freshAction);
    m_themeToolBar->addAction(m_pinkAction);
    m_themeToolBar->addSeparator();

    // 视图工具栏
    m_viewToolBar = new QToolBar(this);
    m_viewToolBar->setObjectName("ViewToolBar");
    m_viewToolBar->addAction(m_zoomInAction);
    m_viewToolBar->addAction(m_zoomOutAction);
    m_viewToolBar->addAction(m_resetZoomAction);

    // 挂载到主窗口
    addToolBar(Qt::TopToolBarArea, m_defaultToolBar);
    addToolBar(Qt::TopToolBarArea, m_fileToolBar);
    addToolBar(Qt::TopToolBarArea, m_editToolBar);
    addToolBar(Qt::TopToolBarArea, m_themeToolBar);
    addToolBar(Qt::TopToolBarArea, m_viewToolBar);

    // 初始状态只显示默认栏
    m_defaultToolBar->show();
    m_currentToolBar = m_defaultToolBar;
    m_fileToolBar->hide();
    m_themeToolBar->hide();
    m_editToolBar->hide();
    m_viewToolBar->hide();
}

/**
 * @brief       切换显示【文件工具栏】
 */
void MainWindow::showFileToolBar()
{
    if (m_currentToolBar == m_fileToolBar) return;
    if (m_currentToolBar) m_currentToolBar->hide();
    m_fileToolBar->show();
    m_currentToolBar = m_fileToolBar;
    statusBar()->showMessage(tr("文件操作工具栏"), 1000);
}

/**
 * @brief       切换显示【编辑工具栏】
 */
void MainWindow::showEditToolBar()
{
    if (m_currentToolBar == m_editToolBar) return;
    if (m_currentToolBar) m_currentToolBar->hide();
    m_editToolBar->show();
    m_currentToolBar = m_editToolBar;
    statusBar()->showMessage(tr("编辑工具栏（开发中）"), 1000);
}

/**
 * @brief       切换显示【主题工具栏】
 */
void MainWindow::showThemeToolBar()
{
    if (m_currentToolBar == m_themeToolBar) return;
    if (m_currentToolBar) m_currentToolBar->hide();
    m_themeToolBar->show();
    m_currentToolBar = m_themeToolBar;
    statusBar()->showMessage(tr("主题操作工具栏"), 1000);
}

/**
 * @brief       切换显示【视图工具栏】
 */
void MainWindow::showViewToolBar()
{
    if (m_currentToolBar == m_viewToolBar) return;
    if (m_currentToolBar) m_currentToolBar->hide();
    m_viewToolBar->show();
    m_currentToolBar = m_viewToolBar;
    statusBar()->showMessage(tr("视图工具栏（开发中）"), 1000);
}

/**
 * @brief       目录树点击：执行JS代码实现预览页平滑滚动定位
 * @param       item    点击节点
 * @param       column  列号
 */
void MainWindow::onTocItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (!item) return;
    QString anchorId = item->data(0, Qt::UserRole).toString();
    if (!anchorId.isEmpty()) {
        QString js = QString("document.getElementById('%1').scrollIntoView({behavior: 'smooth', block: 'start'});").arg(anchorId);
        m_preView->page()->runJavaScript(js);
    }
}

/**
 * @brief       根据解析出的目录数据渲染左侧树形目录
 * @param       toc 目录项列表
 */
void MainWindow::updateToc(const QList<TocItem> &toc)
{
    if (!m_tocTree) return;
    m_tocTree->clear();
    QMap<int, QTreeWidgetItem*> lastParentByLevel;

    for (const TocItem &item : toc) {
        QTreeWidgetItem *treeItem = new QTreeWidgetItem();
        treeItem->setText(0, item.text);
        treeItem->setData(0, Qt::UserRole, item.anchorId);
        int level = item.level;

        if (level == 1) {
            m_tocTree->addTopLevelItem(treeItem);
            lastParentByLevel[1] = treeItem;
        } else {
            QTreeWidgetItem *parent = lastParentByLevel.value(level - 1);
            if (parent) {
                parent->addChild(treeItem);
            } else {
                m_tocTree->addTopLevelItem(treeItem);
            }
            lastParentByLevel[level] = treeItem;
        }

        for (int lvl = level; lvl <= 6; ++lvl) {
            lastParentByLevel[lvl] = treeItem;
        }
    }
    m_tocTree->expandAll();
}

/**
 * @brief       打开文件对话框并加载Markdown文件
 */
void MainWindow::onOpenAction()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                tr("Open Markdown File"),
                                                QString(),
                                                tr("Markdown Files (*.md *.markdown);;All Files (*)"));
    if (path.isEmpty()) return;
    if (!m_editorManager->openFile(path)) {
        QMessageBox::warning(this, tr("Error"),
                             tr("Cannot open file: %1").arg(path));
    }
}

/**
 * @brief       窗口关闭事件：提前保存会话数据
 * @param       event 关闭事件
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_editorManager) {
        m_editorManager->saveSession();
    }
    event->accept();
}

/**
 * @brief       主窗口析构函数
 */
MainWindow::~MainWindow()
{
    delete ui;
}