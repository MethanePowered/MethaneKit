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

#include <Methane/Graphics/IResource.h>

#include <vulkan/vulkan.hpp>

#include <variant>
#include <vector>

namespace Methane::Graphics
{

struct IContextVK;
struct IResourceVK;

class ResourceViewVK final : public ResourceView
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

    explicit ResourceViewVK(const ResourceView& view_id, IResource::Usage usage);

    [[nodiscard]] const Id&       GetId() const noexcept    { return m_id; }
    [[nodiscard]] IResource::Usage GetUsage() const noexcept { return m_id.usage; }
    [[nodiscard]] IResourceVK&    GetResourceVK() const;

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
    Ref<IResourceVK>           m_vulkan_resource_ref;
    Ptr<ViewDescriptorVariant> m_view_desc_var_ptr;
};

using ResourceViewsVK = std::vector<ResourceViewVK>;

struct IResourceVK : virtual IResource // NOSONAR
{
public:
    using Barrier  = IResource::Barrier;
    using Barriers = IResource::Barriers;
    using State    = IResource::State;
    using ViewVK   = ResourceViewVK;
    using ViewsVK  = ResourceViewsVK;

    [[nodiscard]] virtual const IContextVK&       GetContextVK() const noexcept = 0;
    [[nodiscard]] virtual const vk::DeviceMemory& GetNativeDeviceMemory() const noexcept = 0;
    [[nodiscard]] virtual const vk::Device&       GetNativeDevice() const noexcept = 0;
    [[nodiscard]] virtual const Opt<uint32_t>&    GetOwnerQueueFamilyIndex() const noexcept = 0;

    virtual const Ptr<ResourceViewVK::ViewDescriptorVariant>& InitializeNativeViewDescriptor(const View::Id& view_id) = 0;

    [[nodiscard]] static vk::AccessFlags        GetNativeAccessFlagsByResourceState(ResourceState resource_state);
    [[nodiscard]] static vk::ImageLayout        GetNativeImageLayoutByResourceState(ResourceState resource_state);
    [[nodiscard]] static vk::PipelineStageFlags GetNativePipelineStageFlagsByResourceState(ResourceState resource_state);
};

} // namespace Methane::Graphics
