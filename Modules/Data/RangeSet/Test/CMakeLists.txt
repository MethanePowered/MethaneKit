include(CTest)
include(ParseAndAddCatchTests)

set(TARGET MethaneDataRangeSetTest)

add_executable(${TARGET}
    Main.cpp
    RangeTest.cpp
    RangeSetTest.cpp
)

target_link_libraries(${TARGET}
    MethaneDataRangeSet
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
