CPMAddPackage(
    NAME STB
    GITHUB_REPOSITORY MethanePowered/STB
    GIT_TAG master
)

add_library(STB INTERFACE)
target_include_directories(STB INTERFACE "${STB_SOURCE_DIR}")
