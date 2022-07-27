include(MethaneApplications)
include(MethaneShaders)

set(SIMPLE_VARIANT Simple)
set(UNIFORMS_VARIANT Uniforms)

foreach(VARIANT ${SIMPLE_VARIANT} ${UNIFORMS_VARIANT})

    set(TARGET "MethaneHelloCube${VARIANT}")

    add_methane_application(
        TARGET ${TARGET}
        NAME "Methane Hello Cube ${VARIANT}"
        DESCRIPTION "Tutorial demonstrating colored rotating cube rendering with Methane Kit."
        INSTALL_DIR "Apps"
        SOURCES
            HelloCubeApp.cpp
    )

    if(VARIANT STREQUAL "${UNIFORMS_VARIANT}")
        set(VERTEX_SHADER_DEFINES ":UNIFORMS_BUFFER_ENABLED")
        target_compile_definitions(${TARGET}
            PRIVATE UNIFORMS_BUFFER_ENABLED
        )
    endif()

    add_methane_shaders_source(
        TARGET ${TARGET}
        SOURCE Shaders/HelloCube.hlsl
        VERSION 6_0
        TYPES
            vert=CubeVS${VERTEX_SHADER_DEFINES}
            frag=CubePS
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

endforeach()
