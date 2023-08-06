CPMAddPackage(
    NAME FMT
    GITHUB_REPOSITORY MethanePowered/FMT
    GIT_TAG 10.0.0
    VERSION 10.0.0
)

set_target_properties(fmt
    PROPERTIES
    FOLDER Externals
)
