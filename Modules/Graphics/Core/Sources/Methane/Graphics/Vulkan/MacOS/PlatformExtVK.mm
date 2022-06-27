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

FILE: Methane/Graphics/Vulkan/MacOS/PlatformExtVK.mm
Vulkan platform dependent functions for MacOS.

******************************************************************************/

#include <Methane/Graphics/Vulkan/PlatformVK.h>

#include <Methane/Platform/MacOS/AppViewMT.hh>
#include <Methane/Graphics/Metal/RenderContextAppViewMT.hh>
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

vk::UniqueSurfaceKHR PlatformVK::CreateVulkanSurfaceForWindow(const vk::Instance& vk_instance, const Platform::AppEnvironment& env)
{
    META_FUNCTION_TASK();
    AppViewMT* metal_view = nil;
    if (!env.ns_app_delegate.isViewLoaded)
    {
        // Create temporary application view for Window if it was not created yet
        metal_view = CreateTemporaryAppView(env);
        env.ns_app_delegate.view = metal_view;
    }
    else
    {
        metal_view = static_cast<AppViewMT*>(env.ns_app_delegate.view);
    }

    CAMetalLayer* metal_layer = metal_view.metalLayer;
    return vk_instance.createMetalSurfaceEXTUnique(
        vk::MetalSurfaceCreateInfoEXT(
            vk::MetalSurfaceCreateFlagsEXT{},
            metal_layer
        )
    );
}

} // namespace Methane::Graphics
