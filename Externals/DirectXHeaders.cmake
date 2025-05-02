CPMAddPackage(
    NAME DirectXHeaders
    GITHUB_REPOSITORY MethanePowered/DirectXHeaders
    VERSION 1.615.0
)

set_target_properties(DirectX-Headers DirectX-Guids
    PROPERTIES
    FOLDER Externals
)
