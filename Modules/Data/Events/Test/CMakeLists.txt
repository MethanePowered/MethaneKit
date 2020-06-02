include(CTest)
include(ParseAndAddCatchTests)

set(TARGET MethaneDataEventsTest)

add_executable(${TARGET}
    Main.cpp
    EventsTest.cpp
)

target_compile_definitions(${TARGET}
    PRIVATE
        $<$<NOT:$<CONFIG:Debug>>:CATCH_CONFIG_ENABLE_BENCHMARKING>
)

target_link_libraries(${TARGET}
    PRIVATE
        MethaneDataEvents
        MethaneBuildOptions
        $<$<BOOL:${METHANE_TRACY_PROFILING_ENABLED}>:TracyClient>
        Catch
)

if (METHANE_RUN_TESTS_DURING_BUILD)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CTEST_EXE} -C $<CONFIGURATION> --output-on-failure
    )
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

ParseAndAddCatchTests(${TARGET})
