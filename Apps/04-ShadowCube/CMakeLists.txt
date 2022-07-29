set(TARGET MethaneShadowCube)

include(MethaneApplications)
include(MethaneShaders)
include(MethaneResources)

add_methane_application(
    TARGET ${TARGET}
    NAME "Methane Shadow Cube"
    DESCRIPTION "Tutorial demonstrating shadow and final render passes done with Methane Kit."
    INSTALL_DIR "Apps"
    SOURCES
        ShadowCubeApp.h
        ShadowCubeApp.cpp
        Shaders/ShadowCubeUniforms.h
)

set(TEXTURES_DIR ${RESOURCES_DIR}/Textures)
set(TEXTURES
    ${TEXTURES_DIR}/MethaneBubbles.jpg
    ${TEXTURES_DIR}/MarbleWhite.jpg
)
add_methane_embedded_textures(${TARGET} "${TEXTURES_DIR}" "${TEXTURES}")

add_methane_shaders_source(
    TARGET ${TARGET}
    SOURCE Shaders/ShadowCube.hlsl
    VERSION 6_0
    TYPES
        frag=CubePS:ENABLE_SHADOWS,ENABLE_TEXTURING
        vert=CubeVS:ENABLE_SHADOWS,ENABLE_TEXTURING
        vert=CubeVS:ENABLE_TEXTURING
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
