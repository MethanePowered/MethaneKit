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
    "Methane Hello Triangle"
    "${SOURCES}"
    "${RESOURCES_DIR}"
    "Apps/Tutorials"
)
add_methane_shaders(${TARGET} "${SHADERS_HLSL}")

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps/Tutorials
)

#=================================== SIMPLE ===================================

set(TARGET_SIMPLE ${TARGET}Simple)

add_methane_application(${TARGET_SIMPLE}
    "Methane Hello Triangle"
    "HelloTriangleAppSimple.cpp"
    "${RESOURCES_DIR}"
    "Apps/Tutorials"
)
add_methane_shaders(${TARGET_SIMPLE} "${SHADERS_HLSL}")

set_target_properties(${TARGET_SIMPLE}
    PROPERTIES
    FOLDER Apps/Tutorials
)
