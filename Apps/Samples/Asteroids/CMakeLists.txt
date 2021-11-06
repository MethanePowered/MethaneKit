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
    Shaders/AsteroidUniforms.h
    Shaders/PlanetUniforms.h
    Shaders/SceneConstants.h
)

set(TEXTURES_DIR ${RESOURCES_DIR}/Textures)
set(TEXTURES
    ${TEXTURES_DIR}/Planet/Mars.jpg
    ${TEXTURES_DIR}/SkyBox/Galaxy/PositiveX.jpg
    ${TEXTURES_DIR}/SkyBox/Galaxy/NegativeX.jpg
    ${TEXTURES_DIR}/SkyBox/Galaxy/PositiveY.jpg
    ${TEXTURES_DIR}/SkyBox/Galaxy/NegativeY.jpg
    ${TEXTURES_DIR}/SkyBox/Galaxy/PositiveZ.jpg
    ${TEXTURES_DIR}/SkyBox/Galaxy/NegativeZ.jpg
)

set(SHADERS_HLSL
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Asteroids.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Planet.hlsl
)

add_methane_application(${TARGET}
    "${SOURCES}"
    "${RESOURCES_DIR}"
    "Apps"
    "Methane Asteroids"
    "Asteroids sample demonstrates parallel rendering of the asteroids field observable with interactive camera."
    "${METHANE_COPYRIGHT}"
    "${METHANE_VERSION_SHORT}"
    "${METHANE_VERSION_BUILD}"
)
add_methane_embedded_textures(${TARGET} "${TEXTURES_DIR}" "${TEXTURES}")
add_methane_shaders(${TARGET} "${SHADERS_HLSL}" "6_0")

target_link_libraries(${TARGET}
    PRIVATE
        MethaneAppsCommon
        TaskFlow
)

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps/Samples
)
