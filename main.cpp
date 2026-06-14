/**
 * @file        main.cpp
 * @brief       程序入口函数
 * @details     初始化Qt应用、配置QSettings组织/应用名、启动主窗口
 * @author      我没有会员
 * @date        2026-06-14
 * @version     V1.0
 */
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 配置应用标识，保证QSettings会话读写路径统一
    a.setOrganizationName("MyCompany");
    a.setApplicationName("MarkdownEditor");

    MainWindow w;
    w.show();

    int result = a.exec();
    return result;
}