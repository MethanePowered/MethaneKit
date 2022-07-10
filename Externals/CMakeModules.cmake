CPMAddPackage(
    NAME CMakeModules
    GITHUB_REPOSITORY MethanePowered/CMakeModules
    GIT_TAG 9de6ddab101ee6778bda388ae6a807d18cec91ac # last commit from 'methane' branch
)

list(APPEND CMAKE_MODULE_PATH "${CMakeModules_SOURCE_DIR}")