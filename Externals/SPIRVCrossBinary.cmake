CPMAddPackage(
    NAME SPIRVCrossBinary
    GITHUB_REPOSITORY MethanePowered/SPIRVCrossBinary
    GIT_TAG vulkan-sdk-1.3.268.0
    VERSION 1.3.268.0
)

get_platform_dir(PLATFORM_DIR CPP_EXT)
if(APPLE)
    set(PLATFORM_DIR MacOS)
endif()

set(SPIRV_BINARY_DIR "${SPIRVCrossBinary_SOURCE_DIR}/binaries/${PLATFORM_DIR}" PARENT_SCOPE)
