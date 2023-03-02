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

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps
)