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

FILE: Methane/Graphics/Vulkan/Platform.h
Vulkan platform dependent functions.

******************************************************************************/

#pragma once

#include <Methane/Platform/AppEnvironment.h>

#include <vulkan/vulkan.hpp>

#include <string_view>

namespace Methane::Graphics::Vulkan
{

class Platform
{
public:
    Platform() = delete;

    static const std::vector<std::string_view>& GetVulkanInstanceRequiredExtensions();
    static vk::UniqueSurfaceKHR CreateVulkanSurfaceForWindow(const vk::Instance& instance, const Methane::Platform::AppEnvironment& app_env);

private:
    static std::vector<std::string_view> GetPlatformInstanceExtensions(const std::vector<std::string_view>& platform_instance_extensions);
};

} // namespace Methane::Graphics::Vulkan
