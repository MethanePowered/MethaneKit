include(CodeCoverage)

list(APPEND TEST_TARGETS
    MethaneDataEventsTest
    MethaneDataRangeSetTest
    MethaneDataTypesTest
    MethanePlatformInputTest
    MethaneGraphicsCameraTest
    MethaneGraphicsTypesTest
    MethaneGraphicsRhiTest
    MethaneUserInterfaceTypesTest
)

list(APPEND EXCLUDE_DIRS
    /usr/include/*
    /usr/lib/*
    Build/*
)

if (CPM_SOURCE_CACHE)
    list(APPEND EXCLUDE_DIRS
        "${CPM_SOURCE_CACHE}/*"
    )
endif()

setup_target_for_coverage_lcov(
    NAME MethaneTestCoverage
    EXECUTABLE ctest -j 4 || true # Suppress error on test failures
    DEPENDENCIES ${TEST_TARGETS}
    EXCLUDE ${EXCLUDE_DIRS}
    # Disable specific LCov 2.0 errors:
    #   - 'mismatch' and 'unused' are caused by source files, which are not covered with tests;
    #   - 'inconsistent' is caused by mismatched end line numbers, which GCC 15 writes to the
    #     coverage notes of the Catch2 template test registration functions.
    LCOV_ARGS --ignore-errors=mismatch,unused,inconsistent
)