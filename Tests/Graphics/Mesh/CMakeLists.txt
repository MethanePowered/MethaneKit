set(TARGET MethaneGraphicsMeshTest)

add_executable(${TARGET}
    MeshTestHelpers.hpp
    QuadMeshTest.cpp
    CubeMeshTest.cpp
    SphereMeshTest.cpp
    IcosahedronMeshTest.cpp
    UberMeshTest.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
    MethaneGraphicsMesh
    MethaneBuildOptions
    MethaneMathPrecompiledHeaders
    MethaneTestsCatchHelpers
    $<$<BOOL:${METHANE_TRACY_PROFILING_ENABLED}>:TracyClient>
    Catch2WithMain
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
