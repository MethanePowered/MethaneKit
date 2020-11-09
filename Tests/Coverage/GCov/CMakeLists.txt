include(CodeCoverage)

set(TEST_TARGETS
    MethaneDataEventsTest
    MethaneDataRangeSetTest
    MethanePlatformInputTest
    MethaneGraphicsCameraTest
)

setup_target_for_coverage_lcov(
    NAME MethaneTestCoverage
    EXECUTABLE ctest -j 4
    DEPENDENCIES ${TEST_TARGETS}
    EXCLUDE "/usr/include/*" "/usr/lib/*" "Externals/*" "Build/*"
)