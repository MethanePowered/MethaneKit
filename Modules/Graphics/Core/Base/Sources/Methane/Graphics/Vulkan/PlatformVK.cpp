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

FILE: Methane/Graphics/PlatformVK.cpp
Vulkan platform dependent functions.

******************************************************************************/

#include "PlatformVK.h"

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics
{

std::vector<std::string_view> PlatformVK::GetPlatformInstanceExtensions(const std::vector<std::string_view>& platform_instance_extensions)
{
    std::vector<std::string_view> instance_extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
    };
    instance_extensions.insert(instance_extensions.end(), platform_instance_extensions.begin(), platform_instance_extensions.end());
    return instance_extensions;
}

} // namespace Methane::Graphics
