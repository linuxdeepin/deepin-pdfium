cmake_minimum_required(VERSION 3.15)

set(VERSION "1.0.0" CACHE STRING "PDF rendering library based on PDFium")

# Normalize VERSION format to handle irregular inputs
if(DEFINED VERSION)
    # Extract major.minor.patch from VERSION string (e.g., from "1.5.8+u001" to "1.5.8")
    if(VERSION MATCHES "^([0-9]+\\.[0-9]+\\.[0-9]+)")
        set(VERSION "${CMAKE_MATCH_1}")
    else()
        # If no valid semver found, default to "1.0.0"
        set(VERSION "1.0.0")
        message(WARNING "VERSION does not match semver format, defaulted to ${VERSION}")
    endif()
endif()

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

# Auto-detect Qt version (tries Qt6 first, falls back to Qt5)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
message(STATUS "Found Qt version: ${QT_VERSION_MAJOR}")

# Find Qt with the detected version
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui)

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

set(TARGET_NAME ${PROJECT_NAME})
include(target.cmake)
