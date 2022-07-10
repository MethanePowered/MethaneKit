CPMAddPackage(
    NAME SPIRVCrossBinary
    GITHUB_REPOSITORY MethanePowered/SPIRVCrossBinary
    GIT_TAG b440a00005bf0225b4a815fd3dd2d98bba84d4df # 2020-01-16
)

get_platform_dir(PLATFORM_DIR CPP_EXT)
set(SPIRV_BINARY_DIR "${SPIRVCrossBinary_SOURCE_DIR}/binaries/${PLATFORM_DIR}" PARENT_SCOPE)
