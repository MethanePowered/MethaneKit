include(MethaneApplications)
include(MethaneShaders)

set(SHADERS_HLSL   "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Triangle.hlsl")

#================================== STANDARD ==================================

set(TARGET MethaneHelloTriangle)
set(SOURCES
    HelloTriangleApp.h
    HelloTriangleApp.cpp
)

add_methane_application(${TARGET}
    "${SOURCES}"
    "${RESOURCES_DIR}"
    "Apps/Tutorials"
    "Methane Hello Triangle"
    "Hello World tutorial of colored triangle rendering with Methane Kit."
    "${METHANE_COPYRIGHT}"
    "${METHANE_VERSION_SHORT}"
    "${METHANE_VERSION_BUILD}"
)
add_methane_shaders(${TARGET} "${SHADERS_HLSL}" "6_0")

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps/Tutorials
)

#=================================== SIMPLE ===================================

set(TARGET_SIMPLE ${TARGET}Simple)

add_methane_application(${TARGET_SIMPLE}
    "HelloTriangleAppSimple.cpp"
    "${RESOURCES_DIR}"
    "Apps/Tutorials"
    "Methane Hello Triangle"
    "Hello World tutorial of the colored triangle rendering with Methane Kit."
    "${METHANE_COPYRIGHT}"
    "${METHANE_VERSION_SHORT}"
    "${METHANE_VERSION_BUILD}"
)
add_methane_shaders(${TARGET_SIMPLE} "${SHADERS_HLSL}" "6_0")

set_target_properties(${TARGET_SIMPLE}
    PROPERTIES
    FOLDER Apps/Tutorials
)
