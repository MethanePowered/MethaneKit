CPMAddPackage(
    NAME SPIRVCross
    GITHUB_REPOSITORY MethanePowered/SPIRVCross
    GIT_TAG vulkan-sdk-1.3.296.0
    VERSION 1.3.296.0
    OPTIONS
        "SPIRV_CROSS_STATIC ON"
        "SPIRV_CROSS_SHARED OFF"
        "SPIRV_CROSS_CLI OFF"
        "SPIRV_CROSS_ENABLE_TESTS OFF"
        "SPIRV_CROSS_ENABLE_HLSL ON"
        "SPIRV_CROSS_ENABLE_GLSL ON"
        "SPIRV_CROSS_ENABLE_MSL OFF"
        "SPIRV_CROSS_ENABLE_CPP OFF"
        "SPIRV_CROSS_ENABLE_REFLECT OFF"
        "SPIRV_CROSS_ENABLE_C_API OFF"
        "SPIRV_CROSS_ENABLE_UTIL OFF"
)

set_target_properties(spirv-cross-core spirv-cross-hlsl spirv-cross-glsl
    PROPERTIES
    FOLDER Externals
)