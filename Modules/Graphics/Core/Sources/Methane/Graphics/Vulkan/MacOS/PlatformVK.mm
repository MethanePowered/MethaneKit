/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License"),
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/Vulkan/MacOS/PlatformVK.h
Vulkan platform dependent functions for MacOS.

******************************************************************************/

#include <Methane/Graphics/Vulkan/PlatformVK.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

const std::vector<std::string_view>& PlatformVK::GetVulkanInstanceRequiredExtensions()
{
    META_FUNCTION_TASK();
    static const std::vector<std::string_view> s_instance_extensions = GetPlatformInstanceExtensions({
        VK_EXT_METAL_SURFACE_EXTENSION_NAME
    });
    return s_instance_extensions;
}

vk::SurfaceKHR PlatformVK::CreateVulkanSurfaceForWindow(const vk::Instance& vk_instance, const Platform::AppEnvironment&)
{
    META_FUNCTION_TASK();
    // TODO: CAMetalLayer is required here to create Vulkan Surface
    return vk_instance.createMetalSurfaceEXT(
        vk::MetalSurfaceCreateInfoEXT(
            vk::MetalSurfaceCreateFlagsEXT{}
        )
    );
}

} // namespace Methane::Graphics
