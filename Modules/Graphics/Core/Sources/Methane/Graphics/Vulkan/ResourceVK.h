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

FILE: Methane/Graphics/Vulkan/ResourceVK.h
Vulkan specialization of the resource interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Resource.h>

#include <vulkan/vulkan.hpp>

#include <variant>
#include <vector>

namespace Methane::Graphics
{

struct IResourceVK : virtual Resource // NOSONAR
{
public:
    using Barrier    = Resource::Barrier;
    using Barriers   = Resource::Barriers;
    using State      = Resource::State;

    class LocationVK final : public Location
    {
    public:
        explicit LocationVK(const Location& location);

        [[nodiscard]] IResourceVK&                    GetResourceVK() const noexcept                 { return m_vulkan_resource_ref.get(); }
        [[nodiscard]] const vk::DescriptorBufferInfo* GetNativeDescriptorBufferInfo() const noexcept { return std::get_if<vk::DescriptorBufferInfo>(&m_descriptor_info_var); }
        [[nodiscard]] const vk::DescriptorImageInfo*  GetNativeDescriptorImageInfo() const noexcept  { return std::get_if<vk::DescriptorImageInfo>(&m_descriptor_info_var); }
        [[nodiscard]] const vk::BufferView*           GetNativeDescriptorBufferView() const noexcept { return std::get_if<vk::BufferView>(&m_descriptor_info_var); }

    private:
        using DescriptorInfoVariant = std::variant<void*, vk::DescriptorBufferInfo, vk::DescriptorImageInfo, vk::BufferView>;

        Ref<IResourceVK>      m_vulkan_resource_ref;
        DescriptorInfoVariant m_descriptor_info_var;
    };

    using LocationsVK = std::vector<LocationVK>;

    [[nodiscard]] virtual const vk::DeviceMemory& GetNativeDeviceMemory() const noexcept = 0;
};

} // namespace Methane::Graphics
