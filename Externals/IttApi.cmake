if (POLICY CMP0048)
    cmake_policy(SET CMP0048 OLD) # The `project()` command manages `VERSION` variables
endif()

CPMAddPackage(
    NAME IttApi
    GITHUB_REPOSITORY MethanePowered/IttApi
    VERSION 3.23.0
)

set_target_properties(ittnotify
    PROPERTIES
    FOLDER Externals
)
