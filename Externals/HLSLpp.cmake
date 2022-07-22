CPMAddPackage(
    NAME HLSLpp
    GITHUB_REPOSITORY MethanePowered/HLSLpp
    GIT_TAG bae41f0ac977f9c94cf9f6baddce56ae6aab5c8c # integer division fixes
    VERSION 3.2
)

add_library(HLSLpp INTERFACE)
target_include_directories(HLSLpp INTERFACE ${HLSLpp_SOURCE_DIR}/include)
target_compile_definitions(HLSLpp INTERFACE
    HLSLPP_FEATURE_TRANSFORM # Enable transformation matrices
    HLSLPP_LOGICAL_LAYOUT=0  # Set row-major logical layout
    HLSLPP_COORDINATES=0     # Set left-handed coordinate system
)

if(MSVC)
    target_sources(HLSLpp INTERFACE ${HLSLpp_SOURCE_DIR}/include/hlsl++.natvis)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU") # GCC
    target_compile_options(HLSLpp INTERFACE -Wno-deprecated-copy)
endif()
