set(TARGET MethaneShadowCube)

include(MethaneToolchain)

set(SOURCES
    ShadowCubeApp.h
    ShadowCubeApp.cpp
)

set(SHADERS_HLSL ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ShadowCube.hlsl)
set(IMAGES_DIR ${RESOURCES_DIR}/Images)
set(TEXTURES
    ${IMAGES_DIR}/Textures/MethaneBubbles.jpg
    ${IMAGES_DIR}/Textures/MarbleWhite.jpg
)

add_methane_application(${TARGET}
    "Methane Shadow Cube"
    "${SOURCES}"
    "${RESOURCES_DIR}"
    "${IMAGES_DIR}"
    "${TEXTURES}"
    ""
    "Apps/Tutorials"
)

add_methane_shaders(${TARGET} "${SHADERS_HLSL}")

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps/Tutorials
)
