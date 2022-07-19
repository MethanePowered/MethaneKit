CPMAddPackage(
    NAME STB
    GITHUB_REPOSITORY MethanePowered/STB
    GIT_TAG af1a5bc352164740c1cc1354942b1c6b72eacb8a
)

add_library(STB INTERFACE)
target_include_directories(STB INTERFACE "${STB_SOURCE_DIR}")
