set(TARGET MethaneAsteroids)

include(MethaneApplications)
include(MethaneShaders)
include(MethaneResources)

set(SOURCES
    AsteroidsApp.h
    AsteroidsApp.cpp
    AsteroidsAppController.h
    AsteroidsAppController.cpp
    Asteroid.h
    Asteroid.cpp
    AsteroidsArray.h
    AsteroidsArray.cpp
    Planet.h
    Planet.cpp
)

set(IMAGES_DIR ${RESOURCES_DIR}/Images)
set(TEXTURES
    ${IMAGES_DIR}/Textures/Planet/Mars.jpg
    ${IMAGES_DIR}/Textures/SkyBox/Galaxy/PositiveX.jpg
    ${IMAGES_DIR}/Textures/SkyBox/Galaxy/NegativeX.jpg
    ${IMAGES_DIR}/Textures/SkyBox/Galaxy/PositiveY.jpg
    ${IMAGES_DIR}/Textures/SkyBox/Galaxy/NegativeY.jpg
    ${IMAGES_DIR}/Textures/SkyBox/Galaxy/PositiveZ.jpg
    ${IMAGES_DIR}/Textures/SkyBox/Galaxy/NegativeZ.jpg
)

set(SHADERS_HLSL
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Asteroids.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Planet.hlsl
)

add_methane_application(${TARGET}
    "${SOURCES}"
    "${RESOURCES_DIR}"
    "Apps/Samples"
    "Methane Asteroids"
    "Asteroids sample demonstrates parallel rendering of the asteroids field observable with interactive camera."
    "${METHANE_COPYRIGHT}"
    "${METHANE_VERSION_SHORT}"
    "${METHANE_VERSION_BUILD}"
)
add_methane_embedded_textures(${TARGET} "${IMAGES_DIR}" "${TEXTURES}")
add_methane_shaders(${TARGET} "${SHADERS_HLSL}" "6_0")

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps/Samples
)
