/**
 * @file        editormanager.h
 * @brief       多标签编辑器管理模块头文件
 * @details     基于 QTabWidget 实现多文档标签管理、Markdown解析、预览、文件读写、PDF导出、会话恢复
 * @author      我没有会员
 * @date        2026-06-14
 * @version     V1.0
 */
#ifndef EDITORMANAGER_H
#define EDITORMANAGER_H

#include <QTabWidget>
#include <QPlainTextEdit>
#include <QMap>
#include <QTimer>
#include <QList>

/**
 * @struct      TocItem
 * @brief       Markdown 目录项结构体
 * @param       level       标题层级 1~6
 * @param       text        标题显示文本
 * @param       anchorId    HTML 锚点ID，用于预览页跳转定位
 */
struct TocItem {
    int level;
    QString text;
    QString anchorId;
};

/**
 * @class       EditorManager
 * @brief       多标签编辑器管理器
 * @note        核心能力：多文档标签、文件新建/打开/保存/另存为、Markdown转HTML、实时预览、防抖、PDF导出、会话持久化
 *              采用信号槽解耦UI与解析逻辑，200ms防抖避免高频输入造成主线程卡顿
 */
class EditorManager : public QTabWidget
{
    Q_OBJECT
public:
    /**
     * @brief       构造函数
     * @param       parent  父窗口指针
     */
    explicit EditorManager(QWidget *parent = nullptr);

    /**
     * @brief       析构函数，释放资源、停止定时器、断开信号槽
     */
    ~EditorManager();

    //==================== 文件操作接口 ====================
    /**
     * @brief       新建空白 Markdown 文档
     * @return      新建成功返回 true
     */
    bool newFile();

    /**
     * @brief       打开本地 Markdown 文件
     * @param       path    文件绝对路径
     * @return      打开成功返回 true
     */
    bool openFile(const QString &path);

    /**
     * @brief       保存当前活动文档
     * @return      保存成功返回 true
     */
    bool saveCurrentFile();

    /**
     * @brief       文档另存为(支持 .md / .pdf)
     * @return      另存成功返回 true
     */
    bool saveAs();

    /**
     * @brief       关闭指定索引标签页
     * @param       index   标签页索引
     */
    void closeTab(int index);

    /**
     * @brief       关闭标签前检查文档修改状态，询问是否保存
     * @param       index   标签页索引
     * @return      允许关闭返回 true，取消关闭返回 false
     */
    bool maybeSave(int index);

    /**
     * @brief       将当前文档导出为 PDF 文件
     * @param       filePath    导出PDF保存路径
     * @return      导出成功返回 true
     */
    bool exportToPdf(const QString &filePath);

    //==================== 会话持久化接口 ====================
    /**
     * @brief       保存当前会话：打开的文件、光标位置、滚动条位置
     */
    void saveSession();

    /**
     * @brief       程序启动时恢复上次会话状态
     */
    void restoreSession();

    //==================== 样式与预览配置接口 ====================
    /**
     * @brief       设置预览区域 HTML 对应的 CSS 样式
     * @param       css     样式表字符串
     */
    void setPreviewCss(const QString &css);

    /**
     * @brief       批量更新所有编辑器控件样式表
     * @param       styleSheet  QSS样式字符串
     */
    void updateEditorStyle(const QString &styleSheet);

    /**
     * @brief       获取当前编辑器全局样式表
     * @return      QSS 字符串
     */
    const QString& getEditorStyle() const { return m_editorStyleSheet; }

    /**
     * @brief       主动刷新预览内容
     */
    void refreshPreview() { schedulePreview(); }

private:
    /**
     * @struct      TabData
     * @brief       单个标签页附属数据
     * @param       editor      文本编辑器控件指针
     * @param       filePath    关联文件路径，空代表未保存新文档
     * @param       isModified  文档是否被编辑修改(未保存)
     */
    struct TabData
    {
        QPlainTextEdit *editor;
        QString filePath;
        bool isModified;
    };

    QMap<QPlainTextEdit*, QString>  m_filePathMap;   ///< 编辑器指针 -> 文件路径 映射表
    QVector<TabData>                m_tabData;       ///< 按标签索引存储标签页数据

    QTimer          *m_previewTimer;        ///< 预览防抖定时器，间隔200ms
    QString         m_previewCss;           ///< 预览HTML使用的CSS样式
    QString         m_editorStyleSheet;    ///< 编辑器全局QSS样式表

    //==================== 内部辅助函数 ====================
    /**
     * @brief       更新指定标签页的修改状态
     * @param       index       标签索引
     * @param       modified    是否为已修改状态
     */
    void updateTabModified(int index, bool modified);

    /**
     * @brief       更新标签页标题，已修改文档追加 * 标识
     * @param       index   标签索引
     */
    void setTabTitle(int index);

    /**
     * @brief       获取当前激活标签对应的编辑器控件
     * @return      QPlainTextEdit 指针
     */
    QPlainTextEdit* currentEditor() const;

private slots:
    /**
     * @brief       编辑器文本内容改变触发槽函数
     */
    void onTextChanged();

    /**
     * @brief       切换标签页时触发，刷新预览内容
     * @param       index   新选中标签索引
     */
    void onCurrentTabChanged(int index);

    /**
     * @brief       启动防抖定时器，延迟触发预览生成
     */
    void schedulePreview();

    /**
     * @brief       执行 Markdown -> HTML 转换，并发送预览信号
     */
    void generatePreview();

signals:
    /**
     * @brief       HTML预览内容准备完成信号
     * @param       html    渲染后的HTML字符串
     */
    void previewHtmlReady(const QString &html);

    /**
     * @brief       文档目录解析完成信号
     * @param       toc     目录项列表
     */
    void tocReady(const QList<TocItem> &toc);
};

#endif // EDITORMANAGER_H