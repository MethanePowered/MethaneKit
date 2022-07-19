CPMAddPackage(
    NAME FMT
    GITHUB_REPOSITORY MethanePowered/FMT
    GIT_TAG 8.1.1
    VERSION 8.1.1
)

set_target_properties(fmt
    PROPERTIES
    FOLDER Externals
)
