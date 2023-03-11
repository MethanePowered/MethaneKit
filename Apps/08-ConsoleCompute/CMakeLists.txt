if(APPLE_IOS OR APPLE_TVOS)
    # Console applications are not supported on iOS / tvOS
    return()
endif()

set(TARGET MethaneConsoleCompute)

include(MethaneShaders)

add_executable(${TARGET}
    ConsoleCompute.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        MethaneKit
        component # FTXUI
)

add_methane_shaders_source(
    TARGET ${TARGET}
    SOURCE Shaders/GameOfLife.hlsl
    VERSION 6_0
    TYPES
        comp=MainCS
)

add_methane_shaders_library(${TARGET})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Apps
)

install(
    TARGETS ${TARGET}
    RUNTIME
    DESTINATION Apps
    COMPONENT Runtime
)

# PREREQUISITE_RESOURCES contains compiled Metal shaders library, which should be copied next to executable
get_target_property(COPY_RESOURCES ${TARGET} PREREQUISITE_RESOURCES)
if (COPY_RESOURCES AND NOT COPY_RESOURCES STREQUAL "COPY_RESOURCES-NOTFOUND")

    get_target_property(SHADER_TARGETS ${TARGET} GENERATE_METAL_TARGETS)
    if(NOT SHADER_TARGETS OR SHADER_TARGETS STREQUAL "GENERATE_METAL_TARGETS-NOTFOUND")
        set(SHADER_TARGETS ${TARGET})
    endif()

    add_custom_command(TARGET ${SHADER_TARGETS} POST_BUILD
        COMMENT "Copying prerequisite resources for application ${TARGET}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${COPY_RESOURCES} "$<TARGET_FILE_DIR:${TARGET}>"
    )

    install(
        FILES ${COPY_RESOURCES}
        DESTINATION Apps
        COMPONENT Runtime
    )

endif()