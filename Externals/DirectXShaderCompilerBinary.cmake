include(MethaneModules)

CPMAddPackage(
    NAME DirectXShaderCompilerBinary
    GITHUB_REPOSITORY MethanePowered/DirectXShaderCompilerBinary
    GIT_TAG ae99e6a9291b5081e3270750767d9be1c432b735
    VERSION 1.6.2104
    DOWNLOAD_ONLY YES
)

get_platform_arch_dir(PLATFORM_ARCH_DIR CPP_EXT)
if(APPLE)
    set(PLATFORM_ARCH_DIR MacOS)
endif()

set(DXC_BINARY_DIR "${DirectXShaderCompilerBinary_SOURCE_DIR}/binaries/${PLATFORM_ARCH_DIR}/bin" PARENT_SCOPE)