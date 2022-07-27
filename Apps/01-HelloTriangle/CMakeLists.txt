include(MethaneApplications)
include(MethaneShaders)

set(TARGET MethaneHelloTriangle)

add_methane_application(
    TARGET ${TARGET}
    NAME "Methane Hello Triangle"
    DESCRIPTION "Tutorial demonstrating colored triangle rendering with Methane Kit."
    INSTALL_DIR "Apps"
    SOURCES
        HelloTriangleApp.cpp
)

add_methane_shaders_source(
    TARGET ${TARGET}
    SOURCE Shaders/HelloTriangle.hlsl
    VERSION 6_0
    TYPES
        vert=TriangleVS
        frag=TrianglePS
)

add_methane_shaders_library(${TARGET})

target_link_libraries(${TARGET}
    PRIVATE
        MethaneAppsCommon
)

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps/Tutorials
)
