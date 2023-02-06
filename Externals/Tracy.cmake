CPMAddPackage(
    NAME Tracy
    GITHUB_REPOSITORY MethanePowered/Tracy
    GIT_TAG ba78a788ae1ffd66a044452e459c3e55cea0ffcd # last commit in bugfix/tracy_0-9_instrumentation_semicolon
    #VERSION 0.9
    OPTIONS
        "TRACY_STATIC ON"
        "TRACY_ENABLE ${METHANE_TRACY_PROFILING_ENABLED}"
        "TRACY_ON_DEMAND ${METHANE_TRACY_PROFILING_ON_DEMAND}"
)

set_target_properties(TracyClient
    PROPERTIES
        FOLDER Externals
)
