set(TARGET MethaneGraphicsRhiTest)

add_executable(${TARGET}
    RhiTestHelpers.hpp
    ShaderTest.cpp
    ProgramTest.cpp
    ProgramBindingsTest.cpp
    ComputeContextTest.cpp
    ComputeStateTest.cpp
    ViewStateTest.cpp
    CommandQueueTest.cpp
    FenceTest.cpp
    TransferCommandListTest.cpp
    ComputeCommandListTest.cpp
    BufferTest.cpp
    SamplerTest.cpp
    TextureTest.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        MethaneBuildOptions
        MethaneGraphicsRhiNullImpl
        MethaneGraphicsRhiNull
        TaskFlow
        magic_enum
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
