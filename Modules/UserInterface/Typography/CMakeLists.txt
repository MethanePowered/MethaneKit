set(TARGET MethaneUserInterfaceTypography)

include(MethaneShaders)
get_module_dirs("Methane/UserInterface")

set(SHADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Shaders)

set(HEADERS
    ${INCLUDE_DIR}/Typography.h
    ${INCLUDE_DIR}/Font.h
    ${INCLUDE_DIR}/Text.h
)

set(SOURCES
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
        MethaneGraphicsRhi
        MethaneDataProvider
        MethaneDataEvents
    PRIVATE
        MethaneBuildOptions
        MethaneInstrumentation
        MethanePrecompiledExtraHeaders
        MethaneDataPrimitives
        freetype
)

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
