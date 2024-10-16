include(MethaneApplications)
include(MethaneShaders)
include(MethaneResources)

set(BUFFER_VIEWS_VARIANT BufferViews)
set(ROOT_CONSTANTS_VARIANT RootConstants)

foreach(VARIANT ${BUFFER_VIEWS_VARIANT} ${ROOT_CONSTANTS_VARIANT})

    set(TARGET "MethaneParallelRendering${VARIANT}")

    add_methane_application(
        TARGET ${TARGET}
        NAME "Methane Parallel Rendering (${VARIANT})"
        DESCRIPTION "Tutorial demonstrating parallel-rendering with Methane Kit."
        INSTALL_DIR "Apps"
        SOURCES
            ParallelRenderingApp.h
            ParallelRenderingApp.cpp
            ParallelRenderingAppController.h
            ParallelRenderingAppController.cpp
            Shaders/ParallelRenderingUniforms.h
    )

    if(VARIANT STREQUAL "${ROOT_CONSTANTS_VARIANT}")
        target_compile_definitions(${TARGET}
            PRIVATE ROOT_CONSTANTS_ENABLED
        )
    endif()

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

endforeach()
