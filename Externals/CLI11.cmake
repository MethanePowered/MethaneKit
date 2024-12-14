CPMAddPackage(
    NAME CLI11
    GITHUB_REPOSITORY MethanePowered/CLI11
    GIT_TAG a4e5560c5d05bef5dac0c948fb305195c3b8c254 # main branch commit from 10.12.2024
)

set_target_properties(CLI11
    PROPERTIES
    FOLDER Externals
)
