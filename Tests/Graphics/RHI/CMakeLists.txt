set(TARGET MethaneGraphicsRhiTest)

add_executable(${TARGET}
    RhiTestHelpers.hpp
    RhiSettings.hpp
    ShaderTest.cpp
    ProgramTest.cpp
    ProgramBindingsTest.cpp
    ComputeContextTest.cpp
    ComputeStateTest.cpp
    ViewStateTest.cpp
    CommandQueueTest.cpp
    FenceTest.cpp
    CommandListDebugGroupTest.cpp
    TransferCommandListTest.cpp
    ComputeCommandListTest.cpp
    CommandListSetTest.cpp
    CommandKitTest.cpp
    BufferTest.cpp
    BufferSetTest.cpp
    SamplerTest.cpp
    TextureTest.cpp
    SystemTest.cpp
    DeviceTest.cpp
    RenderContextTest.cpp
    RenderStateTest.cpp
    RenderPatternTest.cpp
    RenderPassTest.cpp
    ResourceBarriersTest.cpp
    RenderCommandListsTest.cpp
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
