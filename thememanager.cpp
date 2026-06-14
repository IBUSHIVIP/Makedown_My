/**
 * @file        thememanager.cpp
 * @brief       主题管理类实现文件
 * @details     封装资源文件读取逻辑，按主题名称匹配对应样式文件
 * @author      我没有会员
 * @date        2026-06-14
 * @version     V1.0
 */
#include "ThemeManager.h"
#include <QFile>
#include <QTextStream>

/**
 * @brief       从Qt资源文件读取文本内容
 * @param       resourcePath  资源路径
 * @return      文件全部文本内容，读取失败返回空串
 */
static QString readFileFromResource(const QString &resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    QTextStream stream(&file);
    return stream.readAll();
}

/**
 * @brief       加载主窗口主题样式表
 * @param       themeName  主题名
 * @return      QSS字符串
 */
QString ThemeManager::loadMainStyle(const QString &themeName)
{
    QString path = QString(":/theme/resource/%1.qss").arg(themeName);
    return readFileFromResource(path);
}

/**
 * @brief       加载编辑器样式表
 * @param       themeName  主题名
 * @return      QSS字符串
 * @note        当前版本由主窗口样式统一控制，此处预留扩展接口
 */
QString ThemeManager::loadEditorStyle(const QString &themeName)
{
    Q_UNUSED(themeName);
    return QString();
}

/**
 * @brief       加载HTML预览对应的CSS样式
 * @param       themeName  主题名
 * @return      CSS字符串
 */
QString ThemeManager::loadPreviewCss(const QString &themeName)
{
    QString path = QString(":/preview_css/resource/%1.css").arg(themeName);
    return readFileFromResource(path);
}