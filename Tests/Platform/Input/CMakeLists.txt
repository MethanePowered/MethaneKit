set(TARGET MethanePlatformInputTest)

add_executable(${TARGET}
    KeyboardTest.cpp
    MouseTest.cpp
)

target_precompile_headers(${TARGET} REUSE_FROM MethanePrecompiledHeaders)

target_link_libraries(${TARGET}
    PRIVATE
        MethanePlatformInput
        MethaneBuildOptions
        MethanePrecompiledHeaders
        $<$<BOOL:${METHANE_TRACY_PROFILING_ENABLED}>:TracyClient>
        Catch2WithMain
        magic_enum
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
