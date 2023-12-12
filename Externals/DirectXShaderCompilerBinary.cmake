include(MethaneModules)

CPMAddPackage(
    NAME DirectXShaderCompilerBinary
    GITHUB_REPOSITORY MethanePowered/DirectXShaderCompilerBinary
    GIT_TAG 0fbff6d950ab975364d1b1afc04f3835734e97d2
    VERSION 1.6.2104.2
)

get_platform_arch_dir(PLATFORM_ARCH_DIR CPP_EXT)
if(APPLE)
    set(PLATFORM_ARCH_DIR MacOS)
endif()

set(DXC_BINARY_DIR "${DirectXShaderCompilerBinary_SOURCE_DIR}/binaries/${PLATFORM_ARCH_DIR}/bin" PARENT_SCOPE)