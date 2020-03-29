set(TARGET MethaneGraphicsUserInterface)

include(MethaneShaders)
include(MethaneResources)

get_module_dirs("Methane/Graphics")

set(HEADERS
    ${INCLUDE_DIR}/UserInterface.h
    ${INCLUDE_DIR}/Font.h
    ${INCLUDE_DIR}/Text.h
    ${INCLUDE_DIR}/Badge.h
    ${INCLUDE_DIR}/HeadsUpDisplay.h
)

set(SOURCES
    ${SOURCES_DIR}/Font.cpp
    ${SOURCES_DIR}/Text.cpp
    ${SOURCES_DIR}/Badge.cpp
    ${SOURCES_DIR}/HeadsUpDisplay.cpp
)

set(HLSL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Text.hlsl
)

set(IMAGES_DIR ${RESOURCES_DIR}/Images)
set(TEXTURES
    ${IMAGES_DIR}/Logo/MethaneLogoNameWatermark.png
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

add_methane_shaders(${TARGET} "${HLSL_SOURCES}" "6_0")
add_methane_embedded_textures(${TARGET} "${IMAGES_DIR}" "${TEXTURES}")

target_link_libraries(${TARGET}
    PUBLIC
        MethaneGraphicsExtensions
    PRIVATE
        MethaneBuildOptions
        MethaneGraphicsPrimitives
        MethaneDataProvider
        MethaneInstrumentation
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
    FOLDER Modules/Graphics
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

