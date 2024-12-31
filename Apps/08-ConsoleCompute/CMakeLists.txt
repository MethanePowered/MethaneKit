if(APPLE_IOS OR APPLE_TVOS)
    # Console applications are not supported on iOS / tvOS
    return()
endif()

set(TARGET MethaneConsoleCompute)

include(MethaneShaders)

add_executable(${TARGET}
    ConsoleApp.h
    ConsoleApp.cpp
    ConsoleComputeApp.h
    ConsoleComputeApp.cpp
    Shaders/GameOfLifeRules.h
)

target_link_libraries(${TARGET}
    PRIVATE
        MethaneKit
        ftxui::component # FTXUI
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

    if(METHANE_GFX_API EQUAL METHANE_GFX_METAL)
        get_target_property(SHADER_TARGET ${TARGET} METAL_LIB_TARGET)
    else()
        set(SHADER_TARGETS ${TARGET})
    endif()

    add_custom_command(TARGET ${SHADER_TARGET} POST_BUILD
        COMMENT "Copying prerequisite resources for application ${TARGET}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${COPY_RESOURCES} "$<TARGET_FILE_DIR:${TARGET}>"
    )

    install(
        FILES ${COPY_RESOURCES}
        DESTINATION Apps
        COMPONENT Runtime
    )

endif()