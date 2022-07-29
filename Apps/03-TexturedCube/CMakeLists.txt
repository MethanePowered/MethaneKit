set(TARGET MethaneTexturedCube)

include(MethaneApplications)
include(MethaneShaders)
include(MethaneResources)

add_methane_application(
    TARGET ${TARGET}
    NAME "Methane Textured Cube"
    DESCRIPTION "Tutorial demonstrating textured rotating cube rendering with Methane Kit."
    INSTALL_DIR "Apps"
    SOURCES
        TexturedCubeApp.h
        TexturedCubeApp.cpp
        Shaders/TexturedCubeUniforms.h
)

set(TEXTURES_DIR ${RESOURCES_DIR}/Textures)
set(TEXTURES ${TEXTURES_DIR}/MethaneBubbles.jpg)
add_methane_embedded_textures(${TARGET} "${TEXTURES_DIR}" "${TEXTURES}")

add_methane_shaders_source(
    TARGET ${TARGET}
    SOURCE Shaders/TexturedCube.hlsl
    VERSION 6_0
    TYPES
        frag=CubePS
        vert=CubeVS
)

add_methane_shaders_library(${TARGET})

target_link_libraries(${TARGET}
    PRIVATE
        MethaneAppsCommon
)

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps
)
