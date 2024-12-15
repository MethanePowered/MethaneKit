CPMAddPackage(
    NAME DirectXHeaders
    GITHUB_REPOSITORY MethanePowered/DirectXHeaders
    VERSION 1.614.1
)

set_target_properties(DirectX-Headers DirectX-Guids
    PROPERTIES
    FOLDER Externals
)
