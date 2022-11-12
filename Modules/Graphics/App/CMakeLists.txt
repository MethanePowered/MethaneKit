set(TARGET MethaneGraphicsApp)

get_module_dirs("Methane/Graphics")

set(HEADERS
    ${INCLUDE_DIR}/App.h
    ${INCLUDE_DIR}/App.hpp
    ${INCLUDE_DIR}/AppBase.h
    ${INCLUDE_DIR}/AppController.h
    ${INCLUDE_DIR}/AppCameraController.h
    ${INCLUDE_DIR}/AppContextController.h
)

set(SOURCES
    ${SOURCES_DIR}/AppBase.cpp
    ${SOURCES_DIR}/AppController.cpp
    ${SOURCES_DIR}/AppCameraController.cpp
    ${SOURCES_DIR}/AppContextController.cpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_include_directories(${TARGET}
    PRIVATE
        Sources
    PUBLIC
        Include
)

target_precompile_headers(${TARGET} REUSE_FROM MethanePrecompiledExtraHeaders)

target_link_libraries(${TARGET}
PUBLIC
    MethaneDataProvider
    MethanePlatformApp
    MethanePlatformUtils
    MethanePlatformInput
    MethaneGraphicsRhi
    MethaneGraphicsExtensions
    MethaneGraphicsPrimitives
    MethaneGraphicsCamera
    MethaneInstrumentation
PRIVATE
    MethaneBuildOptions
    MethanePrecompiledExtraHeaders
)

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Graphics
        PUBLIC_HEADER "${HEADERS}"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

install(TARGETS ${TARGET}
    PUBLIC_HEADER
        DESTINATION ${INCLUDE_DIR}
        COMPONENT Development
    ARCHIVE
        DESTINATION Lib
        COMPONENT Development
)
