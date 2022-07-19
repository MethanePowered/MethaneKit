set(TARGET MethaneDataTypesTest)

add_executable(${TARGET}
    RawVectorTest.cpp
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
