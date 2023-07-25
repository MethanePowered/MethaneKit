if(TARGET Vulkan::Headers)
    # Vulkan::Headers target can be added by find_package(Vulkan) when VULKAN_SDK environment variable defines valid path
    return()
endif()

CPMAddPackage(
    NAME VulkanHeaders
    GITHUB_REPOSITORY MethanePowered/VulkanHeaders
    VERSION 1.3.251
)
