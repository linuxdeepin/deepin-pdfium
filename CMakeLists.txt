cmake_minimum_required(VERSION 3.15)

project(deepin-pdfium 
    VERSION 1.0.0
    DESCRIPTION "PDF rendering library based on PDFium"
    LANGUAGES CXX C
)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# 定义构建宏
add_definitions(-DBUILD_DEEPDF_LIB)

# 查找Qt6依赖包
find_package(Qt6 COMPONENTS Core Gui REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED
    chardet
    lcms2  
    freetype2
    libopenjp2
)

# PDFium 第三方库
add_subdirectory(src/3rdparty/pdfium)

# 主库目标
add_library(${PROJECT_NAME} SHARED
    src/dpdfglobal.cpp
    src/dpdfdoc.cpp  
    src/dpdfpage.cpp
    src/dpdfannot.cpp
)

target_include_directories(${PROJECT_NAME} 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${DEPS_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        pdfium
        Qt6::Core
        Qt6::Gui
        ${DEPS_LIBRARIES}
        z
        jpeg
        icuuc
)

# 安装
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES 
    include/dpdfglobal.h
    include/dpdfdoc.h
    include/dpdfpage.h 
    include/dpdfannot.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/deepin-pdfium
)

# pkg-config
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/deepin-pdfium.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/deepin-pdfium.pc
    @ONLY
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/deepin-pdfium.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
) 