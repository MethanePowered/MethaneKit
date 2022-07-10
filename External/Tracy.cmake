CPMAddPackage(
    NAME Tracy
    GITHUB_REPOSITORY MethanePowered/Tracy
    VERSION 0.8.2.1
    OPTIONS
        "TRACY_ENABLE ${METHANE_TRACY_PROFILING_ENABLED}"
        "TRACY_ON_DEMAND ${METHANE_TRACY_PROFILING_ON_DEMAND}"
)

# Tracy instrumentation interface library
add_library(TracyInstrumentation INTERFACE)
target_include_directories(TracyInstrumentation INTERFACE "${Tracy_SOURCE_DIR}")
