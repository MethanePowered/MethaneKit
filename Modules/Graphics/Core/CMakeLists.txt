add_subdirectory(Interface)
add_subdirectory(Base)

if(METHANE_GFX_API EQUAL METHANE_GFX_DIRECTX)
    add_subdirectory(DirectX)
elseif(METHANE_GFX_API EQUAL METHANE_GFX_METAL)
    add_subdirectory(Metal)
endif()

add_subdirectory(Impl)