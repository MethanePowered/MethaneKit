CPMAddPackage(
    NAME FTXUI
    GITHUB_REPOSITORY MethanePowered/FTXUI
    GIT_TAG 436c237213a75107bea99a41a9672fca6c86de02
    #VERSION 4.0.0
    OPTIONS
        "FTXUI_BUILD_DOCS OFF"
        "FTXUI_BUILD_EXAMPLES OFF"
        "FTXUI_BUILD_TESTS OFF"
        "FTXUI_ENABLE_INSTALL OFF"
)

set_target_properties(screen dom component
    PROPERTIES
    FOLDER Externals
)
