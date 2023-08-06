CPMAddPackage(
    NAME STB
    GITHUB_REPOSITORY MethanePowered/STB
    GIT_TAG master
)

add_library(STB INTERFACE)
target_include_directories(STB INTERFACE "${STB_SOURCE_DIR}")

if(MSVC)
    target_compile_options(STB INTERFACE
        /wd4244 # conversion from 'int' to 'short', possible loss of data (stb_image.h:2224)
    )
endif() # MSVC