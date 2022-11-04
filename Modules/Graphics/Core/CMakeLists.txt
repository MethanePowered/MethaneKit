add_subdirectory(Interface)
add_subdirectory(Base)
add_subdirectory(Impl)

if(METHANE_GFX_API EQUAL METHANE_GFX_METAL)
    add_subdirectory(Metal)
endif()