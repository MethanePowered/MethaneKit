CPMAddPackage(
    NAME CMakeModules
    GITHUB_REPOSITORY MethanePowered/CMakeModules
    GIT_TAG 4ea2e62d063fe4a7e2e28c03d2772537c3a7dccc # last commit from 'methane' branch
    DOWNLOAD_ONLY YES
)

list(APPEND CMAKE_MODULE_PATH "${CMakeModules_SOURCE_DIR}")