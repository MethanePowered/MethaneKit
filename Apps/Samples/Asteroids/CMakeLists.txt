set(TARGET MethaneAsteroids)

include(MethaneToolchain)

set(SOURCES
    AsteroidsApp.h
    AsteroidsApp.cpp
)

set(IMAGES_DIR ${RESOURCES_DIR}/Images)
set(TEXTURES
    ${IMAGES_DIR}/Textures/MethaneBubbles.jpg
    ${IMAGES_DIR}/Textures/SkyBox/Galaxy/GalaxySkyPositiveX.jpg
    ${IMAGES_DIR}/Textures/SkyBox/Galaxy/GalaxySkyNegativeX.jpg
    ${IMAGES_DIR}/Textures/SkyBox/Galaxy/GalaxySkyPositiveY.jpg
    ${IMAGES_DIR}/Textures/SkyBox/Galaxy/GalaxySkyNegativeY.jpg
    ${IMAGES_DIR}/Textures/SkyBox/Galaxy/GalaxySkyPositiveZ.jpg
    ${IMAGES_DIR}/Textures/SkyBox/Galaxy/GalaxySkyNegativeZ.jpg
)
set(SHADERS_HLSL ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Asteroids.hlsl)

add_methane_application(${TARGET}
    "Methane Asteroids"
    "${SOURCES}"
    "${RESOURCES_DIR}"
    "${IMAGES_DIR}"
    "${TEXTURES}"
    ""
    "Apps/Samples"
)

add_methane_shaders(${TARGET} "${SHADERS_HLSL}")

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps/Samples
)
