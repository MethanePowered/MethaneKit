/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/ResourceView.h
Vulkan implementation of the ResourceView.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RHI/IResource.h>
#include <Methane/Graphics/RHI/ResourceView.h>

#include <vulkan/vulkan.hpp>

#include <variant>
#include <vector>

namespace Methane::Graphics::Vulkan
{

struct IContext;
struct IResource;

class ResourceView final
    : public Rhi::ResourceView
{
public:
    template<typename DescType, typename ViewType>
    struct ViewDescriptor
    {
        DescType vk_desc;
        ViewType vk_view;
    };

    using BufferViewDescriptor  = ViewDescriptor<vk::DescriptorBufferInfo, vk::UniqueBufferView>;
    using ImageViewDescriptor   = ViewDescriptor<vk::DescriptorImageInfo, vk::UniqueImageView>;
    using ViewDescriptorVariant = std::variant<BufferViewDescriptor, ImageViewDescriptor>;

    ResourceView(const Rhi::ResourceView& view_id, Rhi::ResourceUsageMask usage);

    [[nodiscard]] const Id&               GetId() const noexcept     { return m_id; }
    [[nodiscard]] Rhi::ResourceUsageMask GetUsage() const noexcept  { return m_id.usage; }
    [[nodiscard]] IResource&              GetVulkanResource() const;

    [[nodiscard]] const BufferViewDescriptor*  GetBufferViewDescriptorPtr() const;
    [[nodiscard]] const BufferViewDescriptor&  GetBufferViewDescriptor() const;
    [[nodiscard]] const ImageViewDescriptor*   GetImageViewDescriptorPtr() const;
    [[nodiscard]] const ImageViewDescriptor&   GetImageViewDescriptor() const;

    [[nodiscard]] const vk::DescriptorBufferInfo* GetNativeDescriptorBufferInfoPtr() const;
    [[nodiscard]] const vk::DescriptorImageInfo*  GetNativeDescriptorImageInfoPtr() const;
    [[nodiscard]] const vk::BufferView*           GetNativeBufferViewPtr() const;
    [[nodiscard]] const vk::ImageView*            GetNativeImageViewPtr() const;
    [[nodiscard]] const vk::BufferView&           GetNativeBufferView() const;
    [[nodiscard]] const vk::ImageView&            GetNativeImageView() const;

private:
    Id                         m_id;
    Ref<IResource>             m_vulkan_resource_ref;
    Ptr<ViewDescriptorVariant> m_view_desc_var_ptr;
};

using ResourceViews = std::vector<ResourceView>;

} // namespace Methane::Graphics::Vulkan
