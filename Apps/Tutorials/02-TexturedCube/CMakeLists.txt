set(TARGET MethaneTexturedCube)

include(Methane)

set(SOURCES
    TexturedCubeApp.h
    TexturedCubeApp.cpp
)

set(SHADERS_HLSL   "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Shaders.hlsl")
set(SHADERS_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Shaders.cfg")

set(IMAGES_DIR ${RESOURCES_DIR}/Images)
set(TEXTURES
    ${IMAGES_DIR}/Textures/MethaneBubbles.jpg
)

add_methane_application(${TARGET}
    "Methane Textured Cube"
    "${SOURCES}"
    "${SHADERS_HLSL}"
    "${SHADERS_CONFIG}"
    "${RESOURCES_DIR}"
    "${IMAGES_DIR}"
    "${TEXTURES}"
    ""
    "Apps/Tutorials"
)

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps/Tutorials
)
