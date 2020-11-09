include(CTest)
include(ParseAndAddCatchTests)

set(TARGET MethaneGraphicsCameraTest)

add_executable(${TARGET}
    Main.cpp
    ArcBallCameraTest.cpp
)

target_precompile_headers(${TARGET} REUSE_FROM MethanePrecompiledHeaders)

target_link_libraries(${TARGET}
    PRIVATE
        MethaneGraphicsCamera
        MethaneBuildOptions
        MethanePrecompiledHeaders
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

ParseAndAddCatchTests(${TARGET})