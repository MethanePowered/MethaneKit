set(TARGET MethaneAsteroids)

include(Methane)

set(SOURCES
    AsteroidsApp.h
    AsteroidsApp.cpp
)

set(SHADERS_HLSL   "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Shaders.hlsl")
set(SHADERS_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Shaders.cfg")

set(IMAGES_DIR ${RESOURCES_DIR}/Images)
set(TEXTURES
    ${IMAGES_DIR}/Textures/MethaneBubbles.jpg
    ${IMAGES_DIR}/Textures/MarbleWhite.jpg
)

add_methane_application(${TARGET}
    "Methane Asteroids"
    "${SOURCES}"
    "${SHADERS_HLSL}"
    "${SHADERS_CONFIG}"
    "${RESOURCES_DIR}"
    "${IMAGES_DIR}"
    "${TEXTURES}"
    ""
    "Apps/Samples"
)

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps/Samples
)
