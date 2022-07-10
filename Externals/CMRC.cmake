CPMAddPackage(
    NAME CMRC
    GITHUB_REPOSITORY MethanePowered/CMRC
    GIT_TAG a64bea50c05594c8e7cf1f08e441bb9507742e2e # last commit from 'master' branch
)

list(APPEND CMAKE_MODULE_PATH "${CMRC_SOURCE_DIR}")