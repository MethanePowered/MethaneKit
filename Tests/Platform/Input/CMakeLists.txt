set(TARGET MethanePlatformInputTest)

add_executable(${TARGET}
    KeyboardTest.cpp
    MouseTest.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        MethanePlatformInputKeyboard
        MethanePlatformInputMouse
        MethaneBuildOptions
        MethanePrecompiledHeaders
        magic_enum
        $<$<BOOL:${METHANE_TRACY_PROFILING_ENABLED}>:TracyClient>
        Catch2WithMain
)

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Tests
)

install(TARGETS ${TARGET}
    RUNTIME
        DESTINATION Tests
        COMPONENT Test
)

include(CatchDiscoverAndRunTests)
