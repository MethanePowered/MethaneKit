set(TARGET MethaneUserInterfaceTypography)

include(MethaneShaders)
get_module_dirs("Methane/UserInterface")

set(SHADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Shaders)

set(HEADERS
    ${INCLUDE_DIR}/Typography.h
    ${INCLUDE_DIR}/FontLibrary.h
    ${INCLUDE_DIR}/Font.h
    ${INCLUDE_DIR}/Text.h
)

set(SOURCES
    ${SOURCES_DIR}/FontChar.h
    ${SOURCES_DIR}/FontChar.cpp
    ${SOURCES_DIR}/FontLibrary.cpp
    ${SOURCES_DIR}/Font.cpp
    ${SOURCES_DIR}/Text.cpp
    ${SOURCES_DIR}/TextMesh.h
    ${SOURCES_DIR}/TextMesh.cpp
    ${SHADERS_DIR}/TextUniforms.h
)

set(HLSL_SOURCES
    ${SHADERS_DIR}/Text.hlsl
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

add_methane_shaders_source(
    TARGET ${TARGET}
    SOURCE Shaders/Text.hlsl
    VERSION 6_0
    TYPES
        frag=TextPS
        vert=TextVS
)

add_methane_shaders_library(${TARGET})

target_link_libraries(${TARGET}
    PUBLIC
        MethaneUserInterfaceTypes
        MethaneDataProvider
        MethaneDataEvents
    PRIVATE
        MethaneBuildOptions
        MethaneInstrumentation
        MethaneMathPrecompiledHeaders
        MethaneDataPrimitives
        freetype
)

if(METHANE_PRECOMPILED_HEADERS_ENABLED)
    target_precompile_headers(${TARGET} REUSE_FROM MethaneGraphicsRhiImpl)
endif()

target_include_directories(${TARGET}
    PRIVATE
        Sources
        Shaders
    PUBLIC
        Include
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/UserInterface
        PUBLIC_HEADER "${HEADERS}"
)

install(TARGETS ${TARGET}
    PUBLIC_HEADER
        DESTINATION ${INCLUDE_DIR}
        COMPONENT Development
    ARCHIVE
        DESTINATION Lib
        COMPONENT Development
)

if(METHANE_TESTS_BUILD_ENABLED)

    set(TEST_TARGET MethaneUserInterfaceNullTypography)

    add_library(${TEST_TARGET} STATIC
        ${HEADERS}
        ${SOURCES}
    )

    target_include_directories(${TEST_TARGET}
        PRIVATE
            Sources
            Shaders
        PUBLIC
            Include
    )

    target_link_libraries(${TEST_TARGET}
        PUBLIC
            MethaneUserInterfaceNullTypes
            MethaneGraphicsRhiNullImpl
            MethaneDataProvider
            MethaneDataEvents
        PRIVATE
            MethaneBuildOptions
            MethaneInstrumentation
            MethaneMathPrecompiledHeaders
            MethaneDataPrimitives
            freetype
    )

    if(METHANE_PRECOMPILED_HEADERS_ENABLED)
        target_precompile_headers(${TEST_TARGET} REUSE_FROM MethaneGraphicsRhiNullImpl)
    endif()

    set_target_properties(${TEST_TARGET}
        PROPERTIES
            FOLDER Tests
    )

endif() # METHANE_TESTS_BUILD_ENABLED