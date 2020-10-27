if (NOT METHANE_CODE_COVERAGE_ENABLED)
    return()
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_subdirectory(GCov)
endif()


