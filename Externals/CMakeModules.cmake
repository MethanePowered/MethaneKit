CPMAddPackage(
    NAME CMakeModules
    GITHUB_REPOSITORY MethanePowered/CMakeModules
    GIT_TAG d344f08587fbb9f37f53232dd6a66f0d9038ae09 # last commit from 'methane' branch
)

list(APPEND CMAKE_MODULE_PATH "${CMakeModules_SOURCE_DIR}")