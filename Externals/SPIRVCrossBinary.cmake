CPMAddPackage(
    NAME SPIRVCrossBinary
    GITHUB_REPOSITORY MethanePowered/SPIRVCrossBinary
    GIT_TAG 0de2aa43453675b493952ff8544969965f5d9114 # 2020-01-16
)

get_platform_dir(PLATFORM_DIR CPP_EXT)
if(APPLE)
    set(PLATFORM_DIR MacOS)
endif()

set(SPIRV_BINARY_DIR "${SPIRVCrossBinary_SOURCE_DIR}/binaries/${PLATFORM_DIR}" PARENT_SCOPE)
