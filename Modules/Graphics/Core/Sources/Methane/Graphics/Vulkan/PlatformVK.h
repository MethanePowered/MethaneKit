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

FILE: Methane/Graphics/Vulkan/PlatformVK.h
Vulkan platform dependent functions.

******************************************************************************/

#pragma once

#include <Methane/Platform/AppEnvironment.h>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics
{

const std::vector<std::string>& GetVulkanInstanceRequiredExtensions();
vk::SurfaceKHR CreateVulkanSurfaceForWindow(const vk::Instance& instance, const Platform::AppEnvironment& app_env);

} // namespace Methane::Graphics
