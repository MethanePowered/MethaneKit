if (POLICY CMP0048)
    # IttApi CMake project(...) does not set VERSION, so CMAKE_PROJECT_VERSION will be set to empty
    cmake_policy(SET CMP0048 NEW) # The `project()` command manages `VERSION` variables
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
