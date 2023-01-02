set(TARGET MethaneKit)

get_module_dirs("Methane")

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIR}/Kit.h
)

add_library(${TARGET} INTERFACE
    ${HEADERS}
)

target_include_directories(${TARGET}
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

target_link_libraries(${TARGET}
    INTERFACE
        MethaneGraphicsTypes
        MethaneGraphicsMesh
        MethaneGraphicsCamera
        MethaneGraphicsRhi
        MethaneGraphicsPrimitives
        MethaneGraphicsApp
        MethaneUserInterfaceTypes
        MethaneUserInterfaceTypography
        MethaneUserInterfaceWidgets
        MethaneUserInterfaceApp
)

if(METHANE_PRECOMPILED_HEADERS_ENABLED)
    target_precompile_headers(${TARGET}
        INTERFACE
            <Methane/Kit.h>
    )
endif()

set(PREREQUISITE_TARGETS
    MethaneGraphicsRhi
    MethaneGraphicsPrimitives
    MethaneUserInterfaceTypography
    MethaneUserInterfaceWidgets
    MethaneUserInterfaceApp
)

add_prerequisite_modules(${TARGET} "${PREREQUISITE_TARGETS}")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS})

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