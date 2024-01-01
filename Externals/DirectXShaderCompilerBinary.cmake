include(MethaneModules)

CPMAddPackage(
    NAME DirectXShaderCompilerBinary
    GITHUB_REPOSITORY MethanePowered/DirectXShaderCompilerBinary
    GIT_TAG 023e6ce2e46416ae2b506682b39b99f36c4e8ebc
    VERSION 1.6.2104.2
)

get_platform_arch_dir(PLATFORM_ARCH_DIR CPP_EXT)
if(APPLE)
    set(PLATFORM_ARCH_DIR MacOS)
endif()

set(DXC_BINARY_DIR "${DirectXShaderCompilerBinary_SOURCE_DIR}/binaries/${PLATFORM_ARCH_DIR}/bin" PARENT_SCOPE)