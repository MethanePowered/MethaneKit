include(MethaneModules)

CPMAddPackage(
    NAME MetalShaderConverterBinary
    GITHUB_REPOSITORY MethanePowered/MetalShaderConverterBinary
    VERSION 1.1
)

get_platform_arch_dir(PLATFORM_ARCH_DIR CPP_EXT)
if(APPLE)
    set(PLATFORM_ARCH_DIR MacOS)
endif()

set(MSC_BINARY_DIR "${MetalShaderConverterBinary_SOURCE_DIR}/binaries/${PLATFORM_ARCH_DIR}/bin" PARENT_SCOPE)
