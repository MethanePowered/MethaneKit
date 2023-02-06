add_subdirectory(Interface)
add_subdirectory(Base)

if(METHANE_GFX_API EQUAL METHANE_GFX_DIRECTX)
    add_subdirectory(DirectX)
elseif(METHANE_GFX_API EQUAL METHANE_GFX_VULKAN)
    add_subdirectory(Vulkan)
elseif(METHANE_GFX_API EQUAL METHANE_GFX_METAL)
    add_subdirectory(Metal)
endif()

if(METHANE_TESTS_BUILD_ENABLED)
    add_subdirectory(Null)
endif()

add_subdirectory(Impl)