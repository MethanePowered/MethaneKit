/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/TextureVK.cpp
Vulkan implementation of the texture interface.

******************************************************************************/

#include "TextureVK.h"
#include "RenderContextVK.h"
#include "DeviceVK.h"
#include "TypesVK.h"

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <algorithm>

namespace Methane::Graphics
{

static vk::ImageViewType GetNativeImageViewType(Texture::DimensionType dimension_type)
{
    META_FUNCTION_TASK();
    switch(dimension_type)
    {
    case Texture::DimensionType::Tex1D:              return vk::ImageViewType::e1D;
    case Texture::DimensionType::Tex1DArray:         return vk::ImageViewType::e1DArray;
    case Texture::DimensionType::Tex2D:              return vk::ImageViewType::e2D;
    case Texture::DimensionType::Tex2DArray:
    case Texture::DimensionType::Tex2DMultisample:   return vk::ImageViewType::e2D;
    case Texture::DimensionType::Cube:               return vk::ImageViewType::eCube;
    case Texture::DimensionType::CubeArray:          return vk::ImageViewType::eCubeArray;
    case Texture::DimensionType::Tex3D:              return vk::ImageViewType::e3D;
    default: META_UNEXPECTED_ARG_RETURN(dimension_type, vk::ImageViewType::e1D);
    }
}

static vk::UniqueImageView CreateNativeImageView(const Texture::Settings& settings, const vk::Device& vk_device, const vk::Image& vk_image)
{
    META_FUNCTION_TASK();
    return vk_device.createImageViewUnique(vk::ImageViewCreateInfo(
        vk::ImageViewCreateFlags(),
        vk_image,
        GetNativeImageViewType(settings.dimension_type),
        TypeConverterVK::PixelFormatToVulkan(settings.pixel_format),
        vk::ComponentMapping(),
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    ));
}

Ptr<Texture> Texture::CreateRenderTarget(const RenderContext& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
{
    META_FUNCTION_TASK();
    return std::make_shared<TextureVK>(dynamic_cast<const RenderContextVK&>(context), settings, descriptor_by_usage);
}

Ptr<Texture> Texture::CreateFrameBuffer(const RenderContext& context, FrameBufferIndex frame_buffer_index, const DescriptorByUsage& descriptor_by_usage)
{
    META_FUNCTION_TASK();
    const RenderContext::Settings& context_settings = context.GetSettings();
    const Settings texture_settings = Settings::FrameBuffer(Dimensions(context_settings.frame_size), context_settings.color_format);
    return std::make_shared<FrameBufferTextureVK>(dynamic_cast<const RenderContextVK&>(context), texture_settings, descriptor_by_usage, frame_buffer_index);
}

Ptr<Texture> Texture::CreateDepthStencilBuffer(const RenderContext& context, const DescriptorByUsage& descriptor_by_usage)
{
    META_FUNCTION_TASK();
    const RenderContext::Settings& context_settings = context.GetSettings();
    const Settings texture_settings = Settings::DepthStencilBuffer(Dimensions(context_settings.frame_size), context_settings.depth_stencil_format);
    return std::make_shared<TextureVK>(dynamic_cast<const RenderContextVK&>(context), texture_settings, descriptor_by_usage);
}

Ptr<Texture> Texture::CreateImage(const Context& context, const Dimensions& dimensions, uint32_t array_length, PixelFormat pixel_format, bool mipmapped, const DescriptorByUsage& descriptor_by_usage)
{
    META_FUNCTION_TASK();
    const Settings texture_settings = Settings::Image(dimensions, array_length, pixel_format, mipmapped, Usage::ShaderRead);
    return std::make_shared<TextureVK>(dynamic_cast<const RenderContextVK&>(context), texture_settings, descriptor_by_usage);
}

Ptr<Texture> Texture::CreateCube(const Context& context, uint32_t dimension_size, uint32_t array_length, PixelFormat pixel_format, bool mipmapped, const DescriptorByUsage& descriptor_by_usage)
{
    META_FUNCTION_TASK();
    const Settings texture_settings = Settings::Cube(dimension_size, array_length, pixel_format, mipmapped, Usage::ShaderRead);
    return std::make_shared<TextureVK>(dynamic_cast<const RenderContextVK&>(context), texture_settings, descriptor_by_usage);
}

// TODO: Temporary constructor, to be removed
TextureVK::TextureVK(const RenderContextVK& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : ResourceVK(context, settings, descriptor_by_usage, {})
{
    META_FUNCTION_TASK();
    InitializeDefaultDescriptors();
}

TextureVK::TextureVK(const RenderContextVK& context, const Settings& settings,
                     const DescriptorByUsage& descriptor_by_usage,
                     const vk::Image& vk_image, vk::UniqueImageView&& vk_unique_image_view)
    : ResourceVK(context, settings, descriptor_by_usage, std::move(vk_unique_image_view))
    , m_vk_image(vk_image)
{
    META_FUNCTION_TASK();
    InitializeDefaultDescriptors();
}

void TextureVK::SetData(const SubResources& sub_resources, CommandQueue* sync_cmd_queue)
{
    META_FUNCTION_TASK();
    ResourceVK::SetData(sub_resources, sync_cmd_queue);
    
    if (GetSettings().mipmapped && sub_resources.size() < GetSubresourceCount().GetRawCount())
    {
        GenerateMipLevels();
    }
}

void TextureVK::GenerateMipLevels()
{
    META_FUNCTION_TASK();
}

void TextureVK::ResetNativeImage(const vk::Image& vk_image)
{
    META_FUNCTION_TASK();
    m_vk_image = vk_image;
}

FrameBufferTextureVK::FrameBufferTextureVK(const RenderContextVK& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage,
                                           FrameBufferIndex frame_buffer_index)
    : FrameBufferTextureVK(context, settings, descriptor_by_usage, frame_buffer_index, context.GetNativeFrameImage(frame_buffer_index))
{
    META_FUNCTION_TASK();
}

FrameBufferTextureVK::FrameBufferTextureVK(const RenderContextVK& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage,
                                           FrameBufferIndex frame_buffer_index, const vk::Image& image)
    : TextureVK(context, settings, descriptor_by_usage, image, CreateNativeImageView(settings, context.GetDeviceVK().GetNativeDevice(), image))
    , m_render_context(context)
    , m_frame_buffer_index(frame_buffer_index)
{
    META_FUNCTION_TASK();
}

void FrameBufferTextureVK::ResetNativeImage()
{
    META_FUNCTION_TASK();
    TextureVK::ResetNativeImage(m_render_context.GetNativeFrameImage(m_frame_buffer_index));
    ResetNativeResource(CreateNativeImageView(GetSettings(), m_render_context.GetDeviceVK().GetNativeDevice(), GetNativeImage()));
}

} // namespace Methane::Graphics
