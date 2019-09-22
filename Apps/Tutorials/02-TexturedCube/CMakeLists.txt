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
    "Methane Textured Cube"
    "${SOURCES}"
    "${RESOURCES_DIR}"
    "Apps/Tutorials"
)
add_methane_embedded_textures(${TARGET} "${IMAGES_DIR}" "${TEXTURES}")
add_methane_shaders(${TARGET} "${SHADERS_HLSL}")

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps/Tutorials
)
