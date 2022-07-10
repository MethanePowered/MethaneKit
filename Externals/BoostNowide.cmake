CPMAddPackage(
    NAME BoostNowide
    GITHUB_REPOSITORY MethanePowered/BoostNowide
    GIT_TAG 40666d41d287b9cbb2092f564e8832d3b8c0e1d9 # last commit from 'standalone' branch
    VERSION 11.1.4
    OPTIONS
        "BUILD_SHARED_LIBS OFF"
        "BUILD_TESTING OFF"
        "NOWIDE_INSTALL OFF"
)

set_target_properties(nowide
    PROPERTIES
    FOLDER Externals
)
