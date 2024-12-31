CPMAddPackage(
    NAME BoostNowide
    GITHUB_REPOSITORY MethanePowered/BoostNowide
    VERSION 1.87.0.1 # tag from 'standalone' branch
    OPTIONS
        "BUILD_SHARED_LIBS OFF"
        "BUILD_TESTING OFF"
        "NOWIDE_INSTALL OFF"
)

set_target_properties(nowide
    PROPERTIES
    FOLDER Externals
)
