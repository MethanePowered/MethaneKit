CPMAddPackage(
    NAME IttApi
    GITHUB_REPOSITORY MethanePowered/IttApi
    GIT_TAG 19ab01fb45aa9f4050a22026929417ade689a36a # last commit in 'fix_cmake_policy_0048_warning' branch
    VERSION 3.24.2
)

set_target_properties(ittnotify
    PROPERTIES
    FOLDER Externals
)
