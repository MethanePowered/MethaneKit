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

FILE: Methane/Graphics/Vulkan/Resource.cpp
Vulkan implementation of the resource objects.

******************************************************************************/

#include <Methane/Graphics/Vulkan/ResourceView.h>
#include <Methane/Graphics/Vulkan/Buffer.h>
#include <Methane/Graphics/Vulkan/Texture.h>
#include <Methane/Graphics/Vulkan/Sampler.h>
#include <Methane/Graphics/Vulkan/Types.h>
#include <Methane/Graphics/Vulkan/Utils.hpp>

#include <Methane/Instrumentation.h>
#include <fmt/format.h>

namespace Methane::Graphics::Vulkan
{

ResourceView::ResourceView(const Rhi::ResourceView& view_id, Rhi::ResourceUsageMask usage)
    : Rhi::ResourceView(view_id)
    , m_id(usage, GetSettings())
    , m_vulkan_resource_ref(dynamic_cast<IResource&>(GetResource()))
    , m_view_desc_var_ptr(m_vulkan_resource_ref.get().InitializeNativeViewDescriptor(m_id))
{ }

IResource& ResourceView::GetVulkanResource() const
{
    META_FUNCTION_TASK();
    return m_vulkan_resource_ref.get();
}

const ResourceView::BufferViewDescriptor* ResourceView::GetBufferViewDescriptorPtr() const
{
    META_FUNCTION_TASK();
    META_CHECK_NOT_NULL(m_view_desc_var_ptr);
    return std::get_if<BufferViewDescriptor>(m_view_desc_var_ptr.get());
}

const ResourceView::BufferViewDescriptor& ResourceView::GetBufferViewDescriptor() const
{
    META_FUNCTION_TASK();
    META_CHECK_NOT_NULL(m_view_desc_var_ptr);
    return std::get<BufferViewDescriptor>(*m_view_desc_var_ptr);
}

const ResourceView::ImageViewDescriptor* ResourceView::GetImageViewDescriptorPtr() const
{
    META_FUNCTION_TASK();
    META_CHECK_NOT_NULL(m_view_desc_var_ptr);
    return std::get_if<ImageViewDescriptor>(m_view_desc_var_ptr.get());
}

const ResourceView::ImageViewDescriptor& ResourceView::GetImageViewDescriptor() const
{
    META_FUNCTION_TASK();
    META_CHECK_NOT_NULL(m_view_desc_var_ptr);
    return std::get<ImageViewDescriptor>(*m_view_desc_var_ptr);
}

const vk::DescriptorBufferInfo* ResourceView::GetNativeDescriptorBufferInfoPtr() const
{
    META_FUNCTION_TASK();
    const BufferViewDescriptor* buffer_view_desc_ptr = GetBufferViewDescriptorPtr();
    return buffer_view_desc_ptr ? &buffer_view_desc_ptr->vk_desc : nullptr;
}

const vk::DescriptorImageInfo* ResourceView::GetNativeDescriptorImageInfoPtr() const
{
    META_FUNCTION_TASK();
    const ImageViewDescriptor* image_view_desc_ptr = GetImageViewDescriptorPtr();
    return image_view_desc_ptr ? &image_view_desc_ptr->vk_desc : nullptr;
}

const vk::BufferView* ResourceView::GetNativeBufferViewPtr() const
{
    META_FUNCTION_TASK();
    const BufferViewDescriptor* buffer_view_desc_ptr = GetBufferViewDescriptorPtr();
    return buffer_view_desc_ptr ? &buffer_view_desc_ptr->vk_view.get() : nullptr;
}

const vk::ImageView* ResourceView::GetNativeImageViewPtr() const
{
    META_FUNCTION_TASK();
    const ImageViewDescriptor* image_view_desc_ptr = GetImageViewDescriptorPtr();
    return image_view_desc_ptr ? &image_view_desc_ptr->vk_view.get() : nullptr;
}

const vk::BufferView& ResourceView::GetNativeBufferView() const
{
    META_FUNCTION_TASK();
    const BufferViewDescriptor& buffer_view_desc = GetBufferViewDescriptor();
    META_CHECK_NOT_NULL(buffer_view_desc.vk_view);
    return buffer_view_desc.vk_view.get();
}

const vk::ImageView& ResourceView::GetNativeImageView() const
{
    META_FUNCTION_TASK();
    const ImageViewDescriptor& image_view_desc = GetImageViewDescriptor();
    META_CHECK_NOT_NULL(image_view_desc.vk_view);
    return image_view_desc.vk_view.get();
}

} // namespace Methane::Graphics::Vulkan
