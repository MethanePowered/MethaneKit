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
    MultiOctavePerlinNoise.h
    MultiOctavePerlinNoise.cpp
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

add_methane_shaders_source(
    TARGET ${TARGET}
    SOURCE Shaders/Asteroids.hlsl
    VERSION 6_0
    TYPES
        vert=AsteroidVS:TEXTURES_COUNT=1
        frag=AsteroidPS:TEXTURES_COUNT=1
        vert=AsteroidVS:TEXTURES_COUNT=5
        frag=AsteroidPS:TEXTURES_COUNT=5
        vert=AsteroidVS:TEXTURES_COUNT=10
        frag=AsteroidPS:TEXTURES_COUNT=10
        vert=AsteroidVS:TEXTURES_COUNT=20
        frag=AsteroidPS:TEXTURES_COUNT=20
        vert=AsteroidVS:TEXTURES_COUNT=30
        frag=AsteroidPS:TEXTURES_COUNT=30
        vert=AsteroidVS:TEXTURES_COUNT=40
        frag=AsteroidPS:TEXTURES_COUNT=40
        vert=AsteroidVS:TEXTURES_COUNT=50
        frag=AsteroidPS:TEXTURES_COUNT=50
)

add_methane_shaders_source(
    TARGET ${TARGET}
    SOURCE Shaders/Planet.hlsl
    VERSION 6_0
    TYPES
        frag=PlanetPS
        vert=PlanetVS
)

add_methane_shaders_library(${TARGET})

target_link_libraries(${TARGET}
    PRIVATE
        MethaneAppsCommon
        TaskFlow
        FastNoise
)

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps/Samples
)
