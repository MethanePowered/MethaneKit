set(TARGET MethaneShadowCube)

include(MethaneApplications)
include(MethaneShaders)
include(MethaneResources)

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
    "${SOURCES}"
    "${RESOURCES_DIR}"
    "Apps/Tutorials"
    "Methane Shadow Cube"
    "Tutorial of the shadow pass rendering with Methane Kit."
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
