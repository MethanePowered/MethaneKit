CPMAddPackage(
    NAME Catch2
    GITHUB_REPOSITORY MethanePowered/Catch2
    VERSION 3.0.1
)

list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU") # GCC
    target_compile_options(Catch2
        PUBLIC
            -Wno-error=parentheses
    )
endif()
