CPMAddPackage(
    NAME DirectXHeaders
    GITHUB_REPOSITORY MethanePowered/DirectXHeaders
    VERSION 1.608.2b
)

set_target_properties(DirectX-Headers DirectX-Guids
    PROPERTIES
    FOLDER Externals
)
