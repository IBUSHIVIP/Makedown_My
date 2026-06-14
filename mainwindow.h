/**
 * @file        mainwindow.h
 * @brief       程序主窗口头文件
 * @details     整体布局、菜单栏、工具栏、主题切换、目录跳转、会话管理
 * @author      我没有会员
 * @date        2026-06-14
 * @version     V1.0
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWebEngineView>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QToolBar>
#include "EditorManager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @class       MainWindow
 * @brief       应用主窗口
 * @note        三栏布局：目录树 + 多标签编辑器 + HTML预览；支持多主题切换、目录锚点跳转、程序退出保存会话
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    /**
     * @brief       打开文件菜单项槽函数
     */
    void onOpenAction();

    /**
     * @brief       点击目录树节点，预览页滚动到对应标题位置
     * @param       item    点击的树节点
     * @param       column  列索引
     */
    void onTocItemClicked(QTreeWidgetItem *item, int column);

    /**
     * @brief       接收目录数据，渲染左侧目录树
     * @param       toc     目录项列表
     */
    void updateToc(const QList<TocItem> &toc);

    // 工具栏切换槽函数
    void showFileToolBar();
    void showEditToolBar();
    void showThemeToolBar();
    void showViewToolBar();

protected:
    /**
     * @brief       窗口关闭事件，保存会话数据
     * @param       event   关闭事件对象
     */
    void closeEvent(QCloseEvent *event) override;

private:
    /**
     * @brief       统一创建所有菜单/工具栏动作
     */
    void createActions();

    /**
     * @brief       构建顶部菜单栏
     */
    void createMenuBar();

    /**
     * @brief       构建各类工具栏
     */
    void createToolBars();

    /**
     * @brief       加载并应用指定主题样式
     * @param       themeName  主题名称
     */
    void applyTheme(const QString &themeName);

    Ui::MainWindow *ui;
    EditorManager  *m_editorManager;    ///< 多标签编辑器管理器
    QWebEngineView *m_preView;          ///< HTML预览窗口
    QTreeWidget    *m_tocTree;           ///< 左侧目录树控件

    // 所有功能动作
    QAction *m_newAction;
    QAction *m_openAction;
    QAction *m_saveAction;
    QAction *m_saveAsAction;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_findAction;
    QAction *m_replaceAction;
    QAction *m_lightAction;
    QAction *m_darkAction;
    QAction *m_freshAction;
    QAction *m_pinkAction;
    QAction *m_zoomInAction;
    QAction *m_zoomOutAction;
    QAction *m_resetZoomAction;

    // 工具栏集合
    QToolBar *m_defaultToolBar;
    QToolBar *m_fileToolBar;
    QToolBar *m_editToolBar;
    QToolBar *m_themeToolBar;
    QToolBar *m_viewToolBar;
    QToolBar *m_currentToolBar;         ///< 当前显示的工具栏
};

#endif // MAINWINDOW_H