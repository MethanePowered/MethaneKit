include(MethaneToolchain)

set(SHADERS_HLSL   "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Shaders.hlsl")

#================================== STANDARD ==================================

set(TARGET MethaneHelloTriangle)
set(SOURCES
    HelloTriangleApp.h
    HelloTriangleApp.cpp
)

add_methane_application(${TARGET}
    "Methane Hello Triangle"
    "${SOURCES}"
    "${SHADERS_HLSL}"
    "${RESOURCES_DIR}"
    ""
    ""
    ""
    "Apps/Tutorials"
)

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps/Tutorials
)

#=================================== SIMPLE ===================================

set(TARGET_SIMPLE ${TARGET}Simple)
set(SOURCES_SIMPLE HelloTriangleAppSimple.cpp)

add_methane_application(${TARGET_SIMPLE}
    "Methane Hello Triangle"
    "${SOURCES_SIMPLE}"
    "${SHADERS_HLSL}"
    "${RESOURCES_DIR}"
    ""
    ""
    ""
    "Apps/Tutorials"
)

set_target_properties(${TARGET_SIMPLE}
    PROPERTIES
    FOLDER Apps/Tutorials
)
