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

get_target_property(COPY_RESOURCES ${TARGET} PREREQUISITE_RESOURCES)
add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMENT "Copying prerequisite resources for application ${TARGET}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${COPY_RESOURCES} "$<TARGET_FILE_DIR:${TARGET}>"
)

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

install(
    FILES ${COPY_RESOURCES}
    RUNTIME
        DESTINATION Apps
        COMPONENT Runtime
)