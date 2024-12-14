CPMAddPackage(
    NAME FreeType2
    GITHUB_REPOSITORY MethanePowered/FreeType2
    GIT_TAG VER-2-13-3
    VERSION 2.13.3
    OPTIONS
        "FT_DISABLE_ZLIB ON"
        "FT_DISABLE_BZIP2 ON"
        "FT_DISABLE_PNG ON"
        "FT_DISABLE_HARFBUZZ ON"
        "FT_DISABLE_BROTLI ON"
)

if(MSVC)
    target_compile_options(freetype PRIVATE /wd4267 /wd4244 /wd4018 /wd4312)
else()
    target_compile_options(freetype PRIVATE -Wno-everything)
endif()

set_target_properties(freetype
    PROPERTIES
    FOLDER Externals
)
