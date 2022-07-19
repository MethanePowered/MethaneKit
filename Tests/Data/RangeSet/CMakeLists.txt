set(TARGET MethaneDataRangeSetTest)

add_executable(${TARGET}
    RangeTest.cpp
    RangeSetTest.cpp
)

target_precompile_headers(${TARGET} REUSE_FROM MethanePrecompiledHeaders)

target_link_libraries(${TARGET}
    PRIVATE
        MethaneDataRangeSet
        MethaneBuildOptions
        MethanePrecompiledHeaders
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
