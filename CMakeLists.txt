#====================================================================
# @file        CMakeLists.txt
# @brief       Markdown编辑器项目构建脚本
# @details     基于 CMake + Qt6 实现跨平台编译、资源打包、部署脚本生成
# @author      王展杰
# @date        2026-06-14
# @version     V1.0
#====================================================================

# 指定最低 CMake 版本要求
cmake_minimum_required(VERSION 3.19)

# 定义项目名称与编程语言(C++)
project(Markdown_My LANGUAGES CXX)

# 查找并引入 Qt6 依赖模块
# Core: Qt核心库  Widgets:桌面控件  WebEngineWidgets:网页预览/PDF导出  Concurrent:并发组件
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets WebEngineWidgets Concurrent)

# Qt 标准工程初始化配置
qt_standard_project_setup()

# 开启自动编译 Qt 资源文件(.qrc)
set(CMAKE_AUTORCC ON)

# 定义可执行程序、源码文件、UI文件、资源文件
qt_add_executable(Markdown_My
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    editormanager.h editormanager.cpp
    img.qrc
    thememanager.h thememanager.cpp
    preview_css.qrc
    theme.qrc
)

# 链接 Qt 依赖库
target_link_libraries(Markdown_My
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt6::WebEngineWidgets
        Qt6::Concurrent
)

# 安装配置：区分可执行文件、库文件、Bundle包部署路径
install(TARGETS Markdown_My
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# 生成 Qt 跨平台部署脚本，用于打包发布依赖库
qt_generate_deploy_app_script(
    TARGET Markdown_My
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

# 安装部署脚本
install(SCRIPT ${deploy_script})