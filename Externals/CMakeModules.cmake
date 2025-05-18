CPMAddPackage(
    NAME CMakeModules
    GITHUB_REPOSITORY MethanePowered/CMakeModules
    GIT_TAG methane
    DOWNLOAD_ONLY YES
)

list(APPEND CMAKE_MODULE_PATH "${CMakeModules_SOURCE_DIR}")