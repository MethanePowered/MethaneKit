include(MethaneApplications)
include(MethaneShaders)

set(SIMPLE_VARIANT Simple)
set(UNIFORMS_VARIANT Uniforms)

foreach(VARIANT ${SIMPLE_VARIANT} ${UNIFORMS_VARIANT})

    set(TARGET "MethaneHelloCube${VARIANT}")

    add_methane_application(${TARGET}
        HelloCubeApp.cpp
        "${RESOURCES_DIR}"
        "Apps"
        "Methane Hello Cube ${VARIANT}"
        "Tutorial demonstrating colored rotating cube rendering with Methane Kit."
        "${METHANE_COPYRIGHT}"
        "${METHANE_VERSION_SHORT}"
        "${METHANE_VERSION_BUILD}"
    )

    if(VARIANT STREQUAL "${UNIFORMS_VARIANT}")
        target_compile_definitions(${TARGET}
            PRIVATE UNIFORMS_BUFFER_ENABLED
        )
    endif()

    add_methane_shaders(${TARGET}
        "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/HelloCube.hlsl"
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

endforeach()
