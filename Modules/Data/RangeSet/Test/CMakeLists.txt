include(CTest)
include(ParseAndAddCatchTests)

set(TARGET MethaneRangeSetTest)

add_executable(${TARGET}
    Main.cpp
    RangeTest.cpp
    RangeSetTest.cpp
)

target_link_libraries(${TARGET}
    MethaneRangeSet
    Catch
)

add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMAND ${CTEST_EXE} -C $<CONFIGURATION> --output-on-failure
)

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Tests
)

set(PARSE_CATCH_TESTS_VERBOSE OFF)
ParseAndAddCatchTests(${TARGET})
