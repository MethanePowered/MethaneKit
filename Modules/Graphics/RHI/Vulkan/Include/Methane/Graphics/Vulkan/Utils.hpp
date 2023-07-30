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

FILE: Methane/Graphics/Vulkan/Utils.hpp
Methane graphics utils for Vulkan API.

******************************************************************************/

#pragma once

#include <Methane/Instrumentation.h>

#include <vulkan/vulkan.hpp>

#include <stdint.h>

namespace Methane::Graphics::Vulkan
{

template<typename VulkanObjectType>
void SetVulkanObjectName(const vk::Device& vk_device, const VulkanObjectType& vk_object, const char* name)
{
    META_FUNCTION_TASK();
    if (!vk_object)
        return;

    vk_device.setDebugUtilsObjectNameEXT(
        vk::DebugUtilsObjectNameInfoEXT(
            VulkanObjectType::objectType,
            uint64_t(static_cast<typename VulkanObjectType::CType>(vk_object)), // NOSONAR
            name
        )
    );
}

template<typename VulkanObjectType>
void SetVulkanObjectName(const vk::Device& vk_device, const VulkanObjectType& vk_object, std::string_view name)
{
    SetVulkanObjectName<VulkanObjectType>(vk_device, vk_object, name.data());
}

} // namespace Methane::Graphics