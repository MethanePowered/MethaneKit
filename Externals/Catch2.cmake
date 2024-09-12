CPMAddPackage(
    NAME Catch2
    GITHUB_REPOSITORY MethanePowered/Catch2
    VERSION 3.6.0
)

list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")

set_target_properties(Catch2 Catch2WithMain
    PROPERTIES
    FOLDER Externals
)

# Catch2 headers produce build errors with GCC <= 9, which are muted here
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU") # GCC
    target_compile_options(Catch2
        PUBLIC
            -Wno-parentheses
    )
endif()
