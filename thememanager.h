/**
 * @file        thememanager.h
 * @brief       全局主题管理类头文件
 * @details     统一加载不同主题的QSS样式、预览CSS，从资源文件读取样式内容
 * @author      我没有会员
 * @date        2026-06-14
 * @version     V1.0
 */
#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QString>

/**
 * @class       ThemeManager
 * @brief       主题资源静态管理类
 * @note        纯静态类，负责读取资源文件中各类主题样式
 */
class ThemeManager
{
public:
    /**
     * @brief       加载主窗口全局QSS样式
     * @param       themeName  主题名称
     * @return      QSS样式字符串
     */
    static QString loadMainStyle(const QString &themeName);

    /**
     * @brief       加载编辑器独立QSS样式
     * @param       themeName  主题名称
     * @return      QSS样式字符串
     */
    static QString loadEditorStyle(const QString &themeName);

    /**
     * @brief       加载HTML预览页面CSS样式
     * @param       themeName  主题名称
     * @return      CSS样式字符串
     */
    static QString loadPreviewCss(const QString &themeName);
};

#endif // THEMEMANAGER_H