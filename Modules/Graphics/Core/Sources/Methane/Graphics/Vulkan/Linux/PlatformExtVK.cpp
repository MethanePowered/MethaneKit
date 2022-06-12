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

FILE: Methane/Graphics/Vulkan/Linux/PlatformExtVK.cpp
Vulkan platform dependent functions for Linux.

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
        VK_KHR_XCB_SURFACE_EXTENSION_NAME
    });
    return s_instance_extensions;
}

vk::UniqueSurfaceKHR PlatformVK::CreateVulkanSurfaceForWindow(const vk::Instance& instance, const Platform::AppEnvironment& env)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(env.connection);
    return instance.createXcbSurfaceKHRUnique(vk::XcbSurfaceCreateInfoKHR({}, env.connection, env.window));
}

} // namespace Methane::Graphics
