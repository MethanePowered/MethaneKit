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

    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMENT "Copying prerequisite resources for application ${TARGET}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${COPY_RESOURCES} "$<TARGET_FILE_DIR:${TARGET}>"
    )

    install(
        FILES ${COPY_RESOURCES}
        DESTINATION Apps
        COMPONENT Runtime
    )

endif()