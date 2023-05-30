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
    Externals/*
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
)