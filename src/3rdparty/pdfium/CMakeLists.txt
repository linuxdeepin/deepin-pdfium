add_library(pdfium STATIC)

# 基本定义
target_compile_definitions(pdfium 
    PRIVATE
        # thrid_party 定义
        DEFINE_PS_TABLES_DATA
)

# 架构相关定义
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    target_compile_definitions(pdfium PRIVATE 
        _FX_CPU_=_FX_X64_
        ARCH_CPU_ARM64
    )
    target_compile_options(pdfium PRIVATE -fPIC)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "mips64")
    target_compile_definitions(pdfium PRIVATE _MIPS_ARCH_LOONGSON)
    target_compile_options(pdfium PRIVATE 
        -O3 
        -ftree-vectorize 
        -march=loongson3a 
        -mhard-float 
        -mno-micromips 
        -mno-mips16 
        -flax-vector-conversions 
        -mloongson-ext2 
        -mloongson-mmi
    )
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "loongarch64")
    target_compile_definitions(pdfium PRIVATE _MIPS_ARCH_LOONGSON)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "sw_64|sw64")
    target_compile_definitions(pdfium PRIVATE ARCH_CPU_SW64)
    target_compile_options(pdfium PRIVATE -fPIC)
endif()

file(GLOB_RECURSE PUB_HDR
    ${CMAKE_CURRENT_SOURCE_DIR}/pdfium/public/*.h
)

# fpdfsdk 
file(GLOB_RECURSE SDK_HDR
    ${CMAKE_CURRENT_SOURCE_DIR}/pdfium/fpdfsdk/*.h
)
file(GLOB_RECURSE SDK_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/pdfium/fpdfsdk/*.cpp
)

# core
file(GLOB_RECURSE CORE_HDR
    ${CMAKE_CURRENT_SOURCE_DIR}/pdfium/core/*.h
)
file(GLOB_RECURSE CORE_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/pdfium/core/*.cpp
)

# # 源文件
# target_sources(pdfium
#     PRIVATE
#         # Public headers
#         ${PUB_HDRS}
# )

# 包含子模块
include(fpdfsdk.cmake)
include(core.cmake) 
include(fx_agg.cmake)
# include(fxjs.cmake)
include(fx_skia.cmake)
include(fx_base.cmake)

target_include_directories(pdfium
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/pdfium
)

# 查找系统依赖
find_package(PkgConfig REQUIRED)
pkg_check_modules(PDFIUM_DEPS REQUIRED
    libopenjp2
    lcms2
    freetype2
    zlib
    libpng
    libjpeg
)

# 链接系统依赖
target_link_libraries(pdfium
    PRIVATE
        ${PDFIUM_DEPS_LIBRARIES}
        icuuc
)

target_include_directories(pdfium
    PRIVATE
        ${PDFIUM_DEPS_INCLUDE_DIRS}
) 
