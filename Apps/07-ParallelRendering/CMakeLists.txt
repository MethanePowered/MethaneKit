set(TARGET MethaneParallelRendering)

include(MethaneApplications)
include(MethaneShaders)
include(MethaneResources)

add_methane_application(
    TARGET ${TARGET}
    NAME "Methane Parallel Rendering"
    DESCRIPTION "Tutorial demonstrating parallel-rendering with Methane Kit."
    INSTALL_DIR "Apps"
    SOURCES
        ParallelRenderingApp.h
        ParallelRenderingApp.cpp
        ParallelRenderingAppController.h
        ParallelRenderingAppController.cpp
        Shaders/ParallelRenderingUniforms.h
)

add_methane_shaders_source(
    TARGET ${TARGET}
    SOURCE Shaders/ParallelRendering.hlsl
    VERSION 6_0
    TYPES
        frag=CubePS
        vert=CubeVS
)

add_methane_shaders_library(${TARGET})

target_link_libraries(${TARGET}
    PRIVATE
        MethaneAppsCommon
)

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps
)
