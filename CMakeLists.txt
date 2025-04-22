cmake_minimum_required(VERSION 3.15)

set(VERSION "1.0.0" CACHE STRING "PDF rendering library based on PDFium")

project(deepin-pdfium
    VERSION ${VERSION}
    DESCRIPTION "PDF rendering library based on PDFium"
    LANGUAGES CXX C
)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 编译选项
add_compile_options(
    -fstack-protector-strong
    -D_FORTITY_SOURCE=1
    -z noexecstack
    -pie
    -fPIC
    -Wno-unused-parameter
)

# 添加链接选项
add_link_options(
    -z lazy
)

# 获取系统页大小
execute_process(
    COMMAND getconf PAGESIZE
    OUTPUT_VARIABLE SYSTEM_PAGE_SIZE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_definitions(-DSYSTEMPAGESIZE=${SYSTEM_PAGE_SIZE})

# Qt6必须放前面，否则高版本cmake存在问题
find_package(Qt6 COMPONENTS Core Gui QUIET)
find_package(Qt5 COMPONENTS Core Gui QUIET)

if(NOT Qt5_FOUND AND NOT Qt6_FOUND)
    message(FATAL_ERROR "Neither Qt5 nor Qt6 found - please install at least one")
endif()
find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED
    chardet
    lcms2  
    freetype2
    libopenjp2
)

# PDFium 第三方库
add_subdirectory(src/3rdparty/pdfium)

# 定义导出宏
# add_definitions(-DBUILD_DEEPDF_LIB)

# 源文件列表
set(DEEPDF_SOURCES
    include/dpdfglobal.h
    include/dpdfdoc.h
    include/dpdfpage.h
    include/dpdfannot.h
    src/dpdfglobal.cpp
    src/dpdfdoc.cpp
    src/dpdfpage.cpp
    src/dpdfannot.cpp
)

# 安装
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR}/deepin-pdfium)
set(LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})

# 创建Qt5版本库
if(Qt5_FOUND)
    message(STATUS ">>> Enable Qt5 version of the library")
    set(QT_DESIRED_VERSION 5)
    set(TARGET_NAME ${PROJECT_NAME}5)
    include(target.cmake)
endif()

# 创建Qt6版本库
if(Qt6_FOUND)
    message(STATUS ">>> Enable Qt6 version of the library")
    # keep the same project name for Qt6 in order to compability with other applications, ie. DFM
    set(QT_DESIRED_VERSION 6)
    set(TARGET_NAME ${PROJECT_NAME})
    include(target.cmake)
endif()
