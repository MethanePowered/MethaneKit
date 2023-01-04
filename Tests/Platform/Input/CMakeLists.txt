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
        MethaneMathPrecompiledHeaders
        magic_enum
        $<$<BOOL:${METHANE_TRACY_PROFILING_ENABLED}>:TracyClient>
        Catch2WithMain
)

if(METHANE_PRECOMPILED_HEADERS_ENABLED)
    target_precompile_headers(${TARGET} REUSE_FROM MethaneMathPrecompiledHeaders)
endif()

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
