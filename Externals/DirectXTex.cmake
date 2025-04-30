CPMAddPackage(
    NAME DirectXTex
    GITHUB_REPOSITORY MethanePowered/DirectXTex
    GIT_TAG mar2025
    VERSION 2.0.7
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
