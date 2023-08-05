CPMAddPackage(
    NAME TaskFlow
    GITHUB_REPOSITORY MethanePowered/TaskFlow
    VERSION 3.6.0
    OPTIONS
        "TF_BUILD_BENCHMARKS OFF"
        "TF_BUILD_CUDA OFF"
        "TF_BUILD_SYCL OFF"
        "TF_BUILD_TESTS OFF"
        "TF_BUILD_EXAMPLES OFF"
)

add_library(TaskFlow ALIAS Taskflow)

if(MSVC)
    target_compile_options(Taskflow INTERFACE
        /wd4456 # declaration of 'lock' hides previous local declaration (taskflow/core/executor.hpp:1842)
        /wd4267 # conversion from 'size_t' to 'unsigned char', possible loss of data (taskflow/algorithm/sort.hpp:229)
        /wd4146 # unary minus operator applied to unsigned type, result still unsigned (taskflow/algorithm/sort.hpp:42)
        /wd4244 # conversion from '_Rep' to 'size_t', possible loss of data (taskflow/core/observer.hpp:884)
    )
else() # Clang or GCC
    target_compile_options(Taskflow INTERFACE
        -Wno-shorten-64-to-32 # implicit conversion loses integer precision: 'unsigned long' to 'int' (taskflow/core/observer.hpp:568:24)
    )
endif()