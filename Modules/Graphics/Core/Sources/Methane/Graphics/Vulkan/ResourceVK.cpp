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

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

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

const vk::BufferView* IResourceVK::LocationVK::GetNativeBufferView() const noexcept
{
    META_FUNCTION_TASK();
    const vk::UniqueBufferView* vk_unique_buffer_view_ptr = std::get_if<vk::UniqueBufferView>(m_view_var_ptr.get());
    return vk_unique_buffer_view_ptr ? &vk_unique_buffer_view_ptr->get() : nullptr;
}

const vk::ImageView* IResourceVK::LocationVK::GetNativeImageView() const noexcept
{
    META_FUNCTION_TASK();
    const vk::UniqueImageView* vk_unique_image_view_ptr = std::get_if<vk::UniqueImageView>(m_view_var_ptr.get());
    return vk_unique_image_view_ptr ? &vk_unique_image_view_ptr->get() : nullptr;
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
    m_view_var_ptr = std::make_shared<ViewVariant>();
    *m_view_var_ptr = GetResourceVK().GetNativeDevice().createImageViewUnique(
        vk::ImageViewCreateInfo(
            vk::ImageViewCreateFlags{},
            dynamic_cast<ITextureVK&>(GetResource()).GetNativeImage(),
            ITextureVK::DimensionTypeToImageViewType(texture_settings.dimension_type),
            TypeConverterVK::PixelFormatToVulkan(texture_settings.pixel_format),
            vk::ComponentMapping(),
            // TODO: add support for levels and layers count
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor,
                                      GetSubresourceIndex().GetMipLevel(),   1U,
                                      GetSubresourceIndex().GetArrayIndex(), 1U)
    ));
    m_descriptor_var = vk::DescriptorImageInfo(
        vk::Sampler(),
        *GetNativeImageView(),
        vk::ImageLayout::eUndefined
    );
}

void IResourceVK::LocationVK::InitSamplerLocation()
{
    META_FUNCTION_TASK();
    META_FUNCTION_NOT_IMPLEMENTED_DESCR("vulkan sampler support is not implemented yet");
}

} // using namespace Methane::Graphics
