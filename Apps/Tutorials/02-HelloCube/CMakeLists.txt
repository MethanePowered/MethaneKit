include(MethaneApplications)
include(MethaneShaders)

set(TARGET MethaneHelloCube)

add_methane_application(${TARGET}
    HelloCubeApp.cpp
    "${RESOURCES_DIR}"
    "Apps"
    "Methane Hello Cube"
    "Tutorial demonstrating colored rotating cube rendering with Methane Kit."
    "${METHANE_COPYRIGHT}"
    "${METHANE_VERSION_SHORT}"
    "${METHANE_VERSION_BUILD}"
)

add_methane_shaders(${TARGET}
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/HelloCube.hlsl"
    "6_0"
)

target_link_libraries(${TARGET}
    PRIVATE
        MethaneAppsCommon
)

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps/Tutorials
)