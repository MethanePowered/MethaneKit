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

FILE: Methane/Graphics/Vulkan/ResourceVK.cpp
Vulkan implementation of the resource objects.

******************************************************************************/

#include "ResourceVK.h"
#include "BufferVK.h"
#include "TextureVK.h"
#include "SamplerVK.h"
#include "TypesVK.h"
#include "UtilsVK.hpp"

#include <Methane/Instrumentation.h>
#include <fmt/format.h>

namespace Methane::Graphics
{

static vk::ImageLayout GetVulkanImageLayoutByUsage(Resource::Usage usage) noexcept
{
    META_FUNCTION_TASK();

    using namespace magic_enum::bitwise_operators;
    if (magic_enum::enum_contains(usage & Resource::Usage::ShaderRead))
        return vk::ImageLayout::eShaderReadOnlyOptimal;

    if (magic_enum::enum_contains(usage & Resource::Usage::ShaderWrite) ||
        magic_enum::enum_contains(usage & Resource::Usage::RenderTarget))
        return vk::ImageLayout::eColorAttachmentOptimal; // TODO: add depth and stencil support

    return vk::ImageLayout::eUndefined;
}

IResourceVK::LocationVK::LocationVK(const Location& location)
    : Location(location)
    , m_vulkan_resource_ref(dynamic_cast<IResourceVK&>(GetResource()))
{
    META_FUNCTION_TASK();
    const Resource::Type resource_type = GetResource().GetResourceType();
    switch(resource_type)
    {
    case Resource::Type::Buffer:  InitBufferLocation();  break;
    case Resource::Type::Texture: InitTextureLocation(); break;
    case Resource::Type::Sampler: InitSamplerLocation(); break;
    default:                      META_UNEXPECTED_ARG(resource_type);
    }
}

IResourceVK& IResourceVK::LocationVK::GetResourceVK() const noexcept
{
    META_FUNCTION_TASK();
    return m_vulkan_resource_ref.get();
}

const vk::DescriptorBufferInfo* IResourceVK::LocationVK::GetNativeDescriptorBufferInfo() const noexcept
{
    META_FUNCTION_TASK();
    return std::get_if<vk::DescriptorBufferInfo>(&m_descriptor_var);
}

const vk::DescriptorImageInfo* IResourceVK::LocationVK::GetNativeDescriptorImageInfo() const noexcept
{
    META_FUNCTION_TASK();
    return std::get_if<vk::DescriptorImageInfo>(&m_descriptor_var);
}

const vk::BufferView* IResourceVK::LocationVK::GetNativeBufferViewPtr() const noexcept
{
    META_FUNCTION_TASK();
    const vk::UniqueBufferView* vk_unique_buffer_view_ptr = std::get_if<vk::UniqueBufferView>(m_view_var_ptr.get());
    return vk_unique_buffer_view_ptr ? &vk_unique_buffer_view_ptr->get() : nullptr;
}

const vk::ImageView* IResourceVK::LocationVK::GetNativeImageViewPtr() const noexcept
{
    META_FUNCTION_TASK();
    const vk::UniqueImageView* vk_unique_image_view_ptr = std::get_if<vk::UniqueImageView>(m_view_var_ptr.get());
    return vk_unique_image_view_ptr ? &vk_unique_image_view_ptr->get() : nullptr;
}

const vk::BufferView& IResourceVK::LocationVK::GetNativeBufferView() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_view_var_ptr);
    return std::get<vk::UniqueBufferView>(*m_view_var_ptr).get();
}

const vk::ImageView& IResourceVK::LocationVK::GetNativeImageView() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_view_var_ptr);
    return std::get<vk::UniqueImageView>(*m_view_var_ptr).get();
}

void IResourceVK::LocationVK::InitBufferLocation()
{
    META_FUNCTION_TASK();
    m_descriptor_var = vk::DescriptorBufferInfo(
        dynamic_cast<BufferVK&>(GetResource()).GetNativeResource(),
        static_cast<vk::DeviceSize>(GetOffset()),
        static_cast<vk::DeviceSize>(GetResource().GetSubResourceDataSize(GetSubresourceIndex()) - GetOffset())
    );
}

void IResourceVK::LocationVK::InitTextureLocation()
{
    META_FUNCTION_TASK();
    const Texture& texture = dynamic_cast<const Texture&>(GetResource());
    const Texture::Settings& texture_settings = texture.GetSettings();
    const vk::Device& vk_device = GetResourceVK().GetNativeDevice();

    m_view_var_ptr = std::make_shared<ViewVariant>();
    *m_view_var_ptr = vk_device.createImageViewUnique(
        vk::ImageViewCreateInfo(
            vk::ImageViewCreateFlags{},
            dynamic_cast<ITextureVK&>(GetResource()).GetNativeImage(),
            ITextureVK::DimensionTypeToImageViewType(texture_settings.dimension_type),
            TypeConverterVK::PixelFormatToVulkan(texture_settings.pixel_format),
            vk::ComponentMapping(),
            // TODO: add support for levels and layers count
            vk::ImageSubresourceRange(ITextureVK::GetNativeImageAspectFlags(texture_settings),
                                      GetSubresourceIndex().GetMipLevel(),   1U,
                                      GetSubresourceIndex().GetArrayIndex(), 1U)
    ));

    SetVulkanObjectName(vk_device, GetNativeImageView(), fmt::format("{} Location", texture.GetName()));

    m_descriptor_var = vk::DescriptorImageInfo(
        vk::Sampler(),
        *GetNativeImageViewPtr(),
        GetVulkanImageLayoutByUsage(texture.GetUsage())
    );
}

void IResourceVK::LocationVK::InitSamplerLocation()
{
    META_FUNCTION_TASK();
    const SamplerVK& sampler = dynamic_cast<const SamplerVK&>(GetResource());
    m_descriptor_var = vk::DescriptorImageInfo(sampler.GetNativeSampler());
}

} // using namespace Methane::Graphics
