CPMAddPackage(
    NAME Catch2
    GITHUB_REPOSITORY MethanePowered/Catch2
    VERSION 3.0.1
)

list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")