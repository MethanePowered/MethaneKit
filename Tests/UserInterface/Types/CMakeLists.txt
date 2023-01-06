set(TARGET MethaneUserInterfaceTypesTest)

add_executable(${TARGET}
    UnitTypeCatchHelpers.hpp
    UnitTypesTest.cpp
    FakePlatformApp.hpp
    ContextTest.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        MethaneBuildOptions
        MethaneGraphicsRhiNullImpl
        MethaneUserInterfaceNullTypes
        MethanePlatformApp
        $<$<BOOL:${METHANE_TRACY_PROFILING_ENABLED}>:TracyClient>
        Catch2WithMain
)

if(METHANE_PRECOMPILED_HEADERS_ENABLED)
    target_precompile_headers(${TARGET} REUSE_FROM MethaneGraphicsRhiNullImpl)
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
