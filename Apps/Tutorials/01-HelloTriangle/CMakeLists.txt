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

set(SHADERS_HLSL
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/HelloTriangle.hlsl"
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
