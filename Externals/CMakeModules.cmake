CPMAddPackage(
    NAME CMakeModules
    GITHUB_REPOSITORY MethanePowered/CMakeModules
    GIT_TAG 09b04c9d113cd3a5ed49711999de68896e352c60 # last commit from 'methane' branch
    DOWNLOAD_ONLY YES
)

list(APPEND CMAKE_MODULE_PATH "${CMakeModules_SOURCE_DIR}")