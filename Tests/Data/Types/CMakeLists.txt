include(CTest)
include(Catch)

set(TARGET MethaneDataTypesTest)

add_executable(${TARGET}
    Main.cpp
    VectorTest.cpp
    PointTest.cpp
    RectSizeTest.cpp
    RectTest.cpp
)

target_precompile_headers(${TARGET} REUSE_FROM MethanePrecompiledExtraHeaders)

target_link_libraries(${TARGET}
    PRIVATE
    MethaneDataTypes
    MethaneBuildOptions
    MethanePrecompiledExtraHeaders
    $<$<BOOL:${METHANE_TRACY_PROFILING_ENABLED}>:TracyClient>
    Catch2
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

catch_discover_tests(${TARGET})
