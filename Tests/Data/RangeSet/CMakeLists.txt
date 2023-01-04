set(TARGET MethaneDataRangeSetTest)

add_executable(${TARGET}
    RangeTest.cpp
    RangeSetTest.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        MethaneDataRangeSet
        MethaneBuildOptions
        MethaneCommonPrecompiledHeaders
        $<$<BOOL:${METHANE_TRACY_PROFILING_ENABLED}>:TracyClient>
        Catch2WithMain
)

if(METHANE_PRECOMPILED_HEADERS_ENABLED)
    target_precompile_headers(${TARGET} REUSE_FROM MethaneCommonPrecompiledHeaders)
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
