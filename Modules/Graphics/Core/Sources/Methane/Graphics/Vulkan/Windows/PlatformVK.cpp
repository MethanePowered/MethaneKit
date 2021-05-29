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

FILE: Methane/Graphics/Vulkan/Windows/PlatformVK.h
Vulkan platform dependent functions for Windows.

******************************************************************************/

#include <Methane/Graphics/Vulkan/PlatformVK.h>
#include <Methane/Instrumentation.h>

#include <Windows.h>

namespace Methane::Graphics
{

const std::vector<std::string>& GetVulkanInstanceRequiredExtensions()
{
    META_FUNCTION_TASK();
    static const std::vector<std::string> s_required_instance_extensions {
        "VK_KHR_surface",
        "VK_KHR_win32_surface"
    };
    return s_required_instance_extensions;
}

vk::SurfaceKHR CreateVulkanSurfaceForWindow(const vk::Instance& vk_instance, const Platform::AppEnvironment& app_env)
{
    META_FUNCTION_TASK();
    return vk_instance.createWin32SurfaceKHR(vk::Win32SurfaceCreateInfoKHR(vk::Win32SurfaceCreateFlagsKHR(), GetModuleHandle(NULL), app_env.window_handle));
}

} // namespace Methane::Graphics
