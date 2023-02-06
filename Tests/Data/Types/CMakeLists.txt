set(TARGET MethaneDataTypesTest)

add_executable(${TARGET}
    RawVectorTest.cpp
    PointTest.cpp
    RectSizeTest.cpp
    RectTest.cpp
    EnumMaskTest.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        MethaneDataTypes
        MethaneBuildOptions
        MethaneMathPrecompiledHeaders
        $<$<BOOL:${METHANE_TRACY_PROFILING_ENABLED}>:TracyClient>
        Catch2WithMain
        magic_enum
)

if(METHANE_PRECOMPILED_HEADERS_ENABLED)
    target_precompile_headers(${TARGET} REUSE_FROM MethaneMathPrecompiledHeaders)
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

include(CatchDiscoverAndRunTests)
