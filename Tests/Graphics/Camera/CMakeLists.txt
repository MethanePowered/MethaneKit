set(TARGET MethaneGraphicsCameraTest)

add_executable(${TARGET}
    ArcBallCameraTest.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        MethaneGraphicsCamera
        MethaneBuildOptions
        MethanePrecompiledHeaders
        MethaneTestsCatchHelpers
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
