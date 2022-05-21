set(TARGET MethaneCubeMapArray)

include(MethaneApplications)
include(MethaneShaders)
include(MethaneResources)

set(SOURCES
    CubeMapArrayApp.h
    CubeMapArrayApp.cpp
    TextureLabeler.h
    TextureLabeler.cpp
    Shaders/CubeMapArrayUniforms.h
)

add_methane_application(${TARGET}
    "${SOURCES}"
    "${RESOURCES_DIR}"
    "Apps"
    "Methane Cube-Map Array"
    "Tutorial demonstrating cube-map array texturing with Methane Kit."
    "${METHANE_COPYRIGHT}"
    "${METHANE_VERSION_SHORT}"
    "${METHANE_VERSION_BUILD}"
)

set(TEXTURES_DIR ${RESOURCES_DIR}/Textures)
set(TEXTURES
    ${TEXTURES_DIR}/SkyBox/Clouds/PositiveX.jpg
    ${TEXTURES_DIR}/SkyBox/Clouds/NegativeX.jpg
    ${TEXTURES_DIR}/SkyBox/Clouds/PositiveY.jpg
    ${TEXTURES_DIR}/SkyBox/Clouds/NegativeY.jpg
    ${TEXTURES_DIR}/SkyBox/Clouds/PositiveZ.jpg
    ${TEXTURES_DIR}/SkyBox/Clouds/NegativeZ.jpg
)

add_methane_embedded_textures(${TARGET} "${TEXTURES_DIR}" "${TEXTURES}")

add_methane_shaders_source(
    TARGET ${TARGET}
    SOURCE Shaders/CubeMapArray.hlsl
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
    FOLDER Apps/Tutorials
)
