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

FILE: Methane/Graphics/Vulkan/Linux/PlatformVK.h
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
    static const std::vector<std::string_view> s_instance_extensions = GetPlatformInstanceExtensions({ });
    return s_instance_extensions;
}

vk::SurfaceKHR PlatformVK::CreateVulkanSurfaceForWindow(const vk::Instance&, const Platform::AppEnvironment&)
{
    META_FUNCTION_TASK();
    META_FUNCTION_NOT_IMPLEMENTED_DESCR("Vulkan surface creation is not implemented for Linux.");
}

} // namespace Methane::Graphics
