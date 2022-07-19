CPMAddPackage(
    NAME DirectXTex
    GITHUB_REPOSITORY MethanePowered/DirectXTex
    GIT_TAG may2022
    VERSION 1.9.6
    OPTIONS
        "BUILD_DX12 ON"
        "BUILD_DX11 OFF"
        "BUILD_TOOLS OFF"
        "BUILD_SAMPLE OFF"
)

set_target_properties(DirectXTex
    PROPERTIES
    FOLDER Externals
)
