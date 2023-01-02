set(TARGET MethaneUserInterfaceTypesTest)

add_executable(${TARGET}
    UnitTypeCatchHelpers.hpp
    UnitTypesTest.cpp
    FakeRenderContext.hpp
    FakePlatformApp.hpp
    ContextTest.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        MethaneBuildOptions
        MethaneUserInterfaceTypes
        MethanePlatformApp
        MethaneGraphicsRhi
        MethaneGraphicsRhiBase
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
