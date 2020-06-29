set(TARGET MethaneUserInterfaceTypography)

include(MethaneShaders)
get_module_dirs("Methane/UserInterface")

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
)

set(HLSL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Text.hlsl
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

add_methane_shaders(${TARGET} "${HLSL_SOURCES}" "6_0")

target_link_libraries(${TARGET}
    PUBLIC
        MethaneGraphicsCore
        MethaneDataProvider
        MethaneDataEvents
    PRIVATE
        MethaneBuildOptions
        MethaneInstrumentation
        MethaneDataPrimitives
        freetype
)

target_include_directories(${TARGET}
    PRIVATE
        Sources
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

