add_library(${TARGET_NAME} SHARED ${DEEPDF_SOURCES})
    
set_target_properties(${TARGET_NAME} PROPERTIES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    OUTPUT_NAME "${TARGET_NAME}"
)

target_include_directories(${TARGET_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${TARGET_NAME}>
    PRIVATE
        ${DEPS_INCLUDE_DIRS}
)

target_link_libraries(${TARGET_NAME}
    PRIVATE
        pdfium
        Qt${QT_DESIRED_VERSION}::Core
        Qt${QT_DESIRED_VERSION}::Gui
        ${DEPS_LIBRARIES}
        z
        jpeg
        icuuc
)

# 安装Qt6版本
install(TARGETS ${TARGET_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# 安装头文件
install(FILES 
    include/dpdfglobal.h
    include/dpdfdoc.h
    include/dpdfpage.h 
    include/dpdfannot.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${TARGET_NAME}
)

# 生成Qt6版本的pkg-config文件
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/deepin-pdfium.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.pc
    @ONLY
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

configure_package_config_file(
    misc/deepin-pdfiumConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}Config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${TARGET_NAME}
    PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}Config.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${TARGET_NAME}
)

install(TARGETS ${TARGET_NAME}
    EXPORT ${TARGET_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT ${TARGET_NAME}Targets
    FILE ${TARGET_NAME}Targets.cmake
    NAMESPACE deepin-pdfium::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${TARGET_NAME}
)