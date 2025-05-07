set(TARGET MethaneKit)

get_module_dirs("Methane")

list(APPEND HEADERS
    ${INCLUDE_DIR}/Kit.h
)
list(APPEND SOURCES
    ${SOURCES_DIR}/Kit.cpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_include_directories(${TARGET}
    PUBLIC
        Include
    PRIVATE
        Sources
)

if(METHANE_PRECOMPILED_HEADERS_ENABLED)
    target_precompile_headers(${TARGET}
        PUBLIC
            <Methane/Kit.h>
    )
endif()

target_link_libraries(${TARGET}
    PUBLIC
        MethaneGraphicsTypes
        MethaneGraphicsMesh
        MethaneGraphicsCamera
        MethaneGraphicsRhiImpl
        MethaneGraphicsPrimitives
        MethaneGraphicsApp
        MethaneUserInterfaceTypes
        MethaneUserInterfaceTypography
        MethaneUserInterfaceWidgets
        MethaneUserInterfaceApp
)

set(PREREQUISITE_TARGETS
    MethaneGraphicsRhiImpl
    MethaneGraphicsPrimitives
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
        DESTINATION ${INSTALL_INCLUDE_DIR}
        COMPONENT Development
    PUBLIC_HEADER
        DESTINATION ${INSTALL_INCLUDE_DIR}
        COMPONENT Development
    ARCHIVE
        DESTINATION lib
        COMPONENT Development
)