set(TARGET MethaneKit)

get_module_dirs("Methane")

set(SOURCES
    ${SOURCES_DIR}/Kit.cpp
)

set(HEADERS
    ${INCLUDE_DIR}/Kit.h
)

add_library(${TARGET} STATIC
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(${TARGET}
    PRIVATE
        Sources
    PUBLIC
        Include
)

target_link_libraries(${TARGET}
    PUBLIC
        MethaneGraphicsTypes
        MethaneGraphicsPrimitives
        MethaneGraphicsMesh
        MethaneGraphicsCamera
        MethaneGraphicsRhi
        MethaneGraphicsExtensions
        MethaneGraphicsApp
        MethaneUserInterfaceTypes
        MethaneUserInterfaceTypography
        MethaneUserInterfaceWidgets
        MethaneUserInterfaceApp
)

target_precompile_headers(${TARGET}
    PUBLIC
        ${HEADERS}
)

set(PREREQUISITE_TARGETS
    MethaneGraphicsRhi
    MethaneGraphicsExtensions
    MethaneUserInterfaceTypography
    MethaneUserInterfaceWidgets
    MethaneUserInterfaceApp
)
add_prerequisite_modules(${TARGET} "${PREREQUISITE_TARGETS}")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules
        PUBLIC_HEADER "${HEADERS}"
        GRAPHICS_API ${METHANE_GFX_API}
        DXC_BINARY_DIR "${DXC_BINARY_DIR}"
        SPIRV_BINARY_DIR "${SPIRV_BINARY_DIR}"
)

install(TARGETS ${TARGET}
    INCLUDES
        DESTINATION ${INCLUDE_DIR}
        COMPONENT Development
    PUBLIC_HEADER
        DESTINATION ${INCLUDE_DIR}
        COMPONENT Development
    ARCHIVE
        DESTINATION Lib
        COMPONENT Development
)