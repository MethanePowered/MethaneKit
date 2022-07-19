set(TARGET MethaneDataEventsTest)

set(SOURCES
    EventWrappers.hpp
    EventsTest.cpp
)

# Events benchmark is disabled in Debug builds to let them run faster
if (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(SOURCES ${SOURCES}
        EventsBenchmark.cpp
    )
endif()

add_executable(${TARGET} ${SOURCES})

target_compile_definitions(${TARGET}
    PRIVATE
        $<$<NOT:$<CONFIG:Debug>>:CATCH_CONFIG_ENABLE_BENCHMARKING>
)

target_precompile_headers(${TARGET} REUSE_FROM MethanePrecompiledHeaders)

target_link_libraries(${TARGET}
    PRIVATE
        MethaneDataEvents
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
