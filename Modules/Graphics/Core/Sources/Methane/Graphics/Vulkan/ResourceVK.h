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

        [[nodiscard]] IResourceVK&                    GetResourceVK() const noexcept;
        [[nodiscard]] const vk::DescriptorBufferInfo* GetNativeDescriptorBufferInfo() const noexcept;
        [[nodiscard]] const vk::DescriptorImageInfo*  GetNativeDescriptorImageInfo() const noexcept;
        [[nodiscard]] const vk::BufferView*           GetNativeBufferViewPtr() const noexcept;
        [[nodiscard]] const vk::ImageView*            GetNativeImageViewPtr() const noexcept;
        [[nodiscard]] const vk::BufferView&           GetNativeBufferView() const;
        [[nodiscard]] const vk::ImageView&            GetNativeImageView() const;

    private:
        using DescriptorVariant = std::variant<void*, vk::DescriptorBufferInfo, vk::DescriptorImageInfo>;
        using ViewVariant = std::variant<void*, vk::UniqueImageView, vk::UniqueBufferView>;

        void InitBufferLocation();
        void InitTextureLocation();
        void InitSamplerLocation();

        Ref<IResourceVK>  m_vulkan_resource_ref;
        DescriptorVariant m_descriptor_var;
        Ptr<ViewVariant>  m_view_var_ptr;
    };

    using LocationsVK = std::vector<LocationVK>;

    [[nodiscard]] virtual const vk::DeviceMemory& GetNativeDeviceMemory() const noexcept = 0;
    [[nodiscard]] virtual const vk::Device&       GetNativeDevice() const noexcept = 0;
};

} // namespace Methane::Graphics
