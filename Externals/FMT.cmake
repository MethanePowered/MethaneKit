CPMAddPackage(
    NAME FMT
    GITHUB_REPOSITORY MethanePowered/FMT
    GIT_TAG 11.1.4
    VERSION 11.1.4
)

set_target_properties(fmt
    PROPERTIES
    FOLDER Externals
)
