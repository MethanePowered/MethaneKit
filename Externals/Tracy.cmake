CPMAddPackage(
    NAME Tracy
    GITHUB_REPOSITORY MethanePowered/Tracy
    VERSION 0.9
    OPTIONS
        "TRACY_STATIC ON"
        "TRACY_ENABLE ${METHANE_TRACY_PROFILING_ENABLED}"
        "TRACY_ON_DEMAND ${METHANE_TRACY_PROFILING_ON_DEMAND}"
)

set_target_properties(TracyClient
    PROPERTIES
        FOLDER Externals
)
