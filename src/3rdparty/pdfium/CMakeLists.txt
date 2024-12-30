add_library(pdfium STATIC)

# 基本定义
target_compile_definitions(pdfium 
    PRIVATE
        __QT__
        PNG_PREFIX
        PNG_USE_READ_MACROS
        BUILD_DEEPIN_PDFIUM_LIB
        DEFINE_PS_TABLES_DATA
        FT_CONFIG_OPTION_ADOBE_GLYPH_LIST
        # 系统库定义
        USE_SYSTEM_LIBJPEG
        USE_SYSTEM_ZLIB
        USE_SYSTEM_LIBPNG
        USE_SYSTEM_ICUUC
        USE_SYSTEM_LCMS2
        USE_SYSTEM_LIBOPENJPEG2
        USE_SYSTEM_FREETYPE
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

# 源文件
target_sources(pdfium
    PRIVATE
        # Public headers
        pdfium/public/cpp/fpdf_deleters.h
        pdfium/public/cpp/fpdf_scopers.h
        pdfium/public/fpdf_annot.h
        pdfium/public/fpdf_attachment.h
        pdfium/public/fpdf_catalog.h
        pdfium/public/fpdf_dataavail.h
        pdfium/public/fpdf_doc.h
        pdfium/public/fpdf_edit.h
        pdfium/public/fpdf_ext.h
        pdfium/public/fpdf_flatten.h
        pdfium/public/fpdf_formfill.h
        pdfium/public/fpdf_fwlevent.h
        pdfium/public/fpdf_javascript.h
        pdfium/public/fpdf_ppo.h
        pdfium/public/fpdf_progressive.h
        pdfium/public/fpdf_save.h
        pdfium/public/fpdf_searchex.h
        pdfium/public/fpdf_signature.h
        pdfium/public/fpdf_structtree.h
        pdfium/public/fpdf_sysfontinfo.h
        pdfium/public/fpdf_text.h
        pdfium/public/fpdf_thumbnail.h
        pdfium/public/fpdf_transformpage.h
        pdfium/public/fpdfview.h
)

# 包含子模块
#include(fx_freetype.cmake)
include(fpdfsdk.cmake)
include(core.cmake) 
#include(fx_libopenjpeg.cmake)
include(fx_agg.cmake)
include(fxjs.cmake)
#include(fx_lcms2.cmake)
include(fx_skia.cmake)
include(fx_base.cmake)

target_include_directories(pdfium
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/pdfium
        ${CMAKE_CURRENT_SOURCE_DIR}/pdfium/third_party/freetype/include
    PRIVATE  
        ${CMAKE_CURRENT_SOURCE_DIR}/pdfium/third_party
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