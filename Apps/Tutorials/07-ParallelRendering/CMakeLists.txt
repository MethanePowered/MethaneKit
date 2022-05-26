set(TARGET MethaneParallelRendering)

include(MethaneApplications)
include(MethaneShaders)
include(MethaneResources)

set(SOURCES
    ParallelRenderingApp.h
    ParallelRenderingApp.cpp
    Shaders/ParallelRenderingUniforms.h
)

add_methane_application(${TARGET}
    "${SOURCES}"
    "${RESOURCES_DIR}"
    "Apps"
    "Methane Parallel Rendering"
    "Tutorial demonstrating parallel-rendering with Methane Kit."
    "${METHANE_COPYRIGHT}"
    "${METHANE_VERSION_SHORT}"
    "${METHANE_VERSION_BUILD}"
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
    FOLDER Apps/Tutorials
)
