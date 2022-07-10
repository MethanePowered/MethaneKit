CPMAddPackage(
    NAME TaskFlow
    GITHUB_REPOSITORY MethanePowered/TaskFlow
    VERSION 3.4.0
    OPTIONS
        "TF_BUILD_BENCHMARKS OFF"
        "TF_BUILD_CUDA OFF"
        "TF_BUILD_TESTS OFF"
        "TF_BUILD_EXAMPLES OFF"
)

add_library(TaskFlow ALIAS Taskflow)

if(MSVC)
    target_compile_options(Taskflow INTERFACE
        /wd4456 # declaration of 'lock' hides previous local declaration (taskflow/core/executor.hpp:1842)
    )
endif()