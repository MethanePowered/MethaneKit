include(MethaneApplications)
include(MethaneShaders)

set(TARGET MethaneHelloTriangle)

add_methane_application(${TARGET}
    "HelloTriangleApp.cpp"
    "${RESOURCES_DIR}"
    "Apps"
    "Methane Hello Triangle"
    "Tutorial demonstrating colored triangle rendering with Methane Kit."
    "${METHANE_COPYRIGHT}"
    "${METHANE_VERSION_SHORT}"
    "${METHANE_VERSION_BUILD}"
)

add_methane_shaders(${TARGET}
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/HelloTriangle.hlsl"
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
