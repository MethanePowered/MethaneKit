set(TARGET MethaneTexturedCube)

include(MethaneApplications)
include(MethaneShaders)
include(MethaneResources)

set(SOURCES
    TexturedCubeApp.h
    TexturedCubeApp.cpp
)

set(SHADERS_HLSL ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Cube.hlsl)
set(IMAGES_DIR ${RESOURCES_DIR}/Images)
set(TEXTURES ${IMAGES_DIR}/Textures/MethaneBubbles.jpg)

add_methane_application(${TARGET}
    "${SOURCES}"
    "${RESOURCES_DIR}"
    "Apps/Tutorials"
    "Methane Textured Cube"
    "Tutorial of the textured cube rendering with Methane Kit."
    "${METHANE_COPYRIGHT}"
    "${METHANE_VERSION_SHORT}"
    "${METHANE_VERSION_BUILD}"
)
add_methane_embedded_textures(${TARGET} "${IMAGES_DIR}" "${TEXTURES}")
add_methane_shaders(${TARGET} "${SHADERS_HLSL}" "6_0")

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps/Tutorials
)
