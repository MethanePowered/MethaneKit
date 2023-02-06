CPMAddPackage(
    NAME CMakeModules
    GITHUB_REPOSITORY MethanePowered/CMakeModules
    GIT_TAG b319f6b6b0b2bc3bd3cfcd51418dc2084872e965 # last commit from 'methane' branch
    DOWNLOAD_ONLY YES
)

list(APPEND CMAKE_MODULE_PATH "${CMakeModules_SOURCE_DIR}")