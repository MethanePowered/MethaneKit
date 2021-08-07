include(MethaneApplications)
include(MethaneShaders)

set(TARGET MethaneHelloCube)

set(SOURCES
    HelloCubeApp.h
    HelloCubeApp.cpp
    Shaders/HelloCubeUniforms.h
)

set(SHADERS_HLSL
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/HelloCube.hlsl"
)

add_methane_application(${TARGET}
    "${SOURCES}"
    "${RESOURCES_DIR}"
    "Apps"
    "Methane Hello Cube"
    "Hello Cube tutorial: colored cube rendering with Methane Kit."
    "${METHANE_COPYRIGHT}"
    "${METHANE_VERSION_SHORT}"
    "${METHANE_VERSION_BUILD}"
)

add_methane_shaders(${TARGET} "${SHADERS_HLSL}" "6_0")

target_link_libraries(${TARGET}
    PRIVATE
        MethaneAppsCommon
)

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps/Tutorials
)