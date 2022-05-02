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

vk::ImageAspectFlags ITextureVK::GetNativeImageAspectFlags(const Texture::Settings& settings)
{
    META_FUNCTION_TASK();
    switch(settings.type)
    {
    case Texture::Type::Texture:
    case Texture::Type::FrameBuffer:        return vk::ImageAspectFlagBits::eColor;
    case Texture::Type::DepthStencilBuffer: return IsDepthFormat(settings.pixel_format)
                                                 ? vk::ImageAspectFlagBits::eDepth
                                                 : vk::ImageAspectFlagBits::eStencil;
    default: META_UNEXPECTED_ARG_DESCR_RETURN(settings.type, vk::ImageAspectFlagBits::eColor, "Unsupported texture type");
    }
}

vk::ImageUsageFlags ITextureVK::GetNativeImageUsageFlags(const Texture::Settings& settings)
{
    META_FUNCTION_TASK();
    vk::ImageUsageFlags usage_flags{};
    switch(settings.type)
    {
    case Texture::Type::Texture:
    case Texture::Type::FrameBuffer:
        usage_flags |= vk::ImageUsageFlagBits::eColorAttachment;
        break;

    case Texture::Type::DepthStencilBuffer:
        usage_flags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
        break;

    default: META_UNEXPECTED_ARG(settings.type);
    }

    using namespace magic_enum::bitwise_operators;
    if (magic_enum::enum_contains(settings.usage_mask & Resource::Usage::ShaderRead))
        usage_flags |= vk::ImageUsageFlagBits::eSampled;

    return usage_flags;
}

static vk::UniqueImageView CreateNativeImageView(const Texture::Settings& settings, const vk::Device& vk_device, const vk::Image& vk_image)
{
    META_FUNCTION_TASK();
    return vk_device.createImageViewUnique(vk::ImageViewCreateInfo(
        vk::ImageViewCreateFlags(),
        vk_image,
        ITextureVK::DimensionTypeToImageViewType(settings.dimension_type),
        TypeConverterVK::PixelFormatToVulkan(settings.pixel_format),
        vk::ComponentMapping(),
        vk::ImageSubresourceRange(ITextureVK::GetNativeImageAspectFlags(settings), 0, 1, 0, 1)
    ));
}

vk::ImageType ITextureVK::DimensionTypeToImageType(Texture::DimensionType dimension_type)
{
    META_FUNCTION_TASK();
    switch(dimension_type)
    {
    case Texture::DimensionType::Tex1D:
    case Texture::DimensionType::Tex1DArray:
        return vk::ImageType::e1D;

    case Texture::DimensionType::Tex2D:
    case Texture::DimensionType::Tex2DArray:
    case Texture::DimensionType::Tex2DMultisample:
        return vk::ImageType::e2D;

    case Texture::DimensionType::Cube:
    case Texture::DimensionType::CubeArray:
    case Texture::DimensionType::Tex3D:
        return vk::ImageType::e3D;

    default: META_UNEXPECTED_ARG_RETURN(dimension_type, vk::ImageType::e1D);
    }
}

vk::ImageViewType ITextureVK::DimensionTypeToImageViewType(Texture::DimensionType dimension_type)
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

Ptr<Texture> Texture::CreateRenderTarget(const RenderContext& render_context, const Settings& settings, const DescriptorByUsage&)
{
    META_FUNCTION_TASK();
    switch (settings.type)
    {
    case Texture::Type::Texture:            return std::make_shared<RenderTargetTextureVK>(dynamic_cast<const RenderContextVK&>(render_context), settings);
    case Texture::Type::DepthStencilBuffer: return std::make_shared<DepthStencilTextureVK>(dynamic_cast<const RenderContextVK&>(render_context), settings,
                                                                                           render_context.GetSettings().clear_depth_stencil);
    case Texture::Type::FrameBuffer: META_UNEXPECTED_ARG_DESCR(settings.type, "frame buffer texture must be created with static method Texture::CreateFrameBuffer");
    default:                         META_UNEXPECTED_ARG_RETURN(settings.type, nullptr);
    }

}

Ptr<Texture> Texture::CreateFrameBuffer(const RenderContext& context, FrameBufferIndex frame_buffer_index, const DescriptorByUsage&)
{
    META_FUNCTION_TASK();
    const RenderContext::Settings& context_settings = context.GetSettings();
    const Settings texture_settings = Settings::FrameBuffer(Dimensions(context_settings.frame_size), context_settings.color_format);
    return std::make_shared<FrameBufferTextureVK>(dynamic_cast<const RenderContextVK&>(context), texture_settings, frame_buffer_index);
}

Ptr<Texture> Texture::CreateDepthStencilBuffer(const RenderContext& context, const DescriptorByUsage&)
{
    META_FUNCTION_TASK();
    const RenderContext::Settings& context_settings = context.GetSettings();
    const Settings texture_settings = Settings::DepthStencilBuffer(Dimensions(context_settings.frame_size), context_settings.depth_stencil_format);
    return std::make_shared<DepthStencilTextureVK>(dynamic_cast<const RenderContextVK&>(context), texture_settings, context_settings.clear_depth_stencil);
}

Ptr<Texture> Texture::CreateImage(const Context& context, const Dimensions& dimensions, uint32_t array_length, PixelFormat pixel_format, bool mipmapped, const DescriptorByUsage&)
{
    META_FUNCTION_TASK();
    const Settings texture_settings = Settings::Image(dimensions, array_length, pixel_format, mipmapped, Usage::ShaderRead);
    return std::make_shared<ImageTextureVK>(dynamic_cast<const RenderContextVK&>(context), texture_settings);
}

Ptr<Texture> Texture::CreateCube(const Context& context, uint32_t dimension_size, uint32_t array_length, PixelFormat pixel_format, bool mipmapped, const DescriptorByUsage&)
{
    META_FUNCTION_TASK();
    const Settings texture_settings = Settings::Cube(dimension_size, array_length, pixel_format, mipmapped, Usage::ShaderRead);
    return std::make_shared<ImageTextureVK>(dynamic_cast<const RenderContextVK&>(context), texture_settings);
}

FrameBufferTextureVK::FrameBufferTextureVK(const RenderContextVK& render_context, const Settings& settings, FrameBufferIndex frame_buffer_index)
    : ResourceVK(render_context, settings, render_context.GetNativeFrameImage(frame_buffer_index))
    , m_render_context(render_context)
    , m_frame_buffer_index(frame_buffer_index)
{
    META_FUNCTION_TASK();
    ResetNativeView(CreateNativeImageView(GetSettings(), GetNativeDevice(), GetNativeResource()));
}

void FrameBufferTextureVK::SetData(const SubResources&, CommandQueue&)
{
    META_FUNCTION_NOT_IMPLEMENTED_DESCR("frame-buffer textures do not support data setup");
}

vk::ImageSubresourceRange FrameBufferTextureVK::GetNativeSubresourceRange() const noexcept
{
    META_FUNCTION_TASK();
    return vk::ImageSubresourceRange(
        vk::ImageAspectFlagBits::eColor,
        0U, 1U,
        0U, 1U
    );
}

void FrameBufferTextureVK::ResetNativeImage()
{
    META_FUNCTION_TASK();
    ResetNativeResource(m_render_context.GetNativeFrameImage(m_frame_buffer_index));
    ResetNativeView(CreateNativeImageView(GetSettings(), m_render_context.GetDeviceVK().GetNativeDevice(), GetNativeResource()));
}

DepthStencilTextureVK::DepthStencilTextureVK(const RenderContextVK& render_context, const Settings& settings,
                                             const Opt<DepthStencil>& depth_stencil_opt)
    : ResourceVK(render_context, settings,
                 render_context.GetDeviceVK().GetNativeDevice().createImageUnique(
                     vk::ImageCreateInfo(
                         vk::ImageCreateFlags{},
                         vk::ImageType::e2D,
                         TypeConverterVK::PixelFormatToVulkan(settings.pixel_format),
                         TypeConverterVK::DimensionsToExtent3D(settings.dimensions),
                         1U, // mip-levels count
                         1U, // array length
                         vk::SampleCountFlagBits::e1,
                         vk::ImageTiling::eOptimal,
                         ITextureVK::GetNativeImageUsageFlags(settings),
                         vk::SharingMode::eExclusive)))
    , m_render_context(render_context)
    , m_depth_stencil_opt(depth_stencil_opt)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL_DESCR(settings.dimension_type, Texture::DimensionType::Tex2D, "depth-stencil texture is supported only with 2D dimensions");
    META_CHECK_ARG_EQUAL_DESCR(settings.dimensions.GetDepth(), 1U, "depth-stencil texture does not support 3D dimensions");
    META_CHECK_ARG_FALSE_DESCR(settings.mipmapped, "depth-stencil texture does not support mip-map mode");
    META_CHECK_ARG_EQUAL_DESCR(settings.array_length, 1U, "depth-stencil texture does not support arrays");

    // Allocate resource primary memory
    const vk::Device& vk_device = GetNativeDevice();
    AllocateResourceMemory(vk_device.getImageMemoryRequirements(GetNativeResource()), vk::MemoryPropertyFlagBits::eDeviceLocal);
    vk_device.bindImageMemory(GetNativeResource(), GetNativeDeviceMemory(), 0);

    ResetNativeView(CreateNativeImageView(GetSettings(), GetNativeDevice(), GetNativeResource()));
}

void DepthStencilTextureVK::SetData(const SubResources&, CommandQueue&)
{
    META_FUNCTION_NOT_IMPLEMENTED_DESCR("depth-stencil textures do not support data setup");
}

vk::ImageSubresourceRange DepthStencilTextureVK::GetNativeSubresourceRange() const noexcept
{
    META_FUNCTION_TASK();
    return vk::ImageSubresourceRange(
        vk::ImageAspectFlagBits::eDepth,
        0U, 1U,
        0U, 1U
    );
}

RenderTargetTextureVK::RenderTargetTextureVK(const RenderContextVK& render_context, const Settings& settings)
    : ResourceVK(render_context, settings, {}) // TODO: initialize native resource
    , m_render_context(render_context)
{
    META_FUNCTION_TASK();
}

void RenderTargetTextureVK::SetData(const SubResources&, CommandQueue&)
{
    META_FUNCTION_NOT_IMPLEMENTED_DESCR("render-target textures do not support data setup");
}

vk::ImageSubresourceRange RenderTargetTextureVK::GetNativeSubresourceRange() const noexcept
{
    META_FUNCTION_TASK();
    const SubResource::Count& subresource_count = GetSubresourceCount();
    return vk::ImageSubresourceRange(
        vk::ImageAspectFlagBits::eColor, // ?
        0U, subresource_count.GetMipLevelsCount(),
        0U, subresource_count.GetArraySize()
    );
}

ImageTextureVK::ImageTextureVK(const ContextBase& context, const Settings& settings)
    : ResourceVK(context, settings,
                 dynamic_cast<const IContextVK&>(context).GetDeviceVK().GetNativeDevice().createImageUnique(
                     vk::ImageCreateInfo(
                         vk::ImageCreateFlags{},
                         ITextureVK::DimensionTypeToImageType(settings.dimension_type),
                         TypeConverterVK::PixelFormatToVulkan(settings.pixel_format),
                         TypeConverterVK::DimensionsToExtent3D(settings.dimensions),
                         settings.mipmapped ? GetRequiredMipLevelsCount(settings.dimensions) : 1U,
                         settings.array_length,
                         vk::SampleCountFlagBits::e1,
                         vk::ImageTiling::eOptimal,
                         vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                         vk::SharingMode::eExclusive)))
{
    META_FUNCTION_TASK();

    // Allocate resource primary memory
    const vk::Device& vk_device = GetNativeDevice();
    const vk::MemoryRequirements vk_image_memory_requirements = vk_device.getImageMemoryRequirements(GetNativeResource());
    AllocateResourceMemory(vk_image_memory_requirements, vk::MemoryPropertyFlagBits::eDeviceLocal);
    vk_device.bindImageMemory(GetNativeResource(), GetNativeDeviceMemory(), 0);

    // Create staging buffer and allocate staging memory
    m_vk_unique_staging_buffer = vk_device.createBufferUnique(
        vk::BufferCreateInfo(vk::BufferCreateFlags{},
                             vk_image_memory_requirements.size,
                             vk::BufferUsageFlagBits::eTransferSrc,
                             vk::SharingMode::eExclusive)
    );

    const vk::MemoryPropertyFlags vk_staging_memory_flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    m_vk_unique_staging_memory = AllocateDeviceMemory(vk_device.getBufferMemoryRequirements(m_vk_unique_staging_buffer.get()), vk_staging_memory_flags);
    vk_device.bindBufferMemory(m_vk_unique_staging_buffer.get(), m_vk_unique_staging_memory.get(), 0);
}

void ImageTextureVK::SetData(const SubResources& sub_resources, CommandQueue& target_cmd_queue)
{
    META_FUNCTION_TASK();
    ResourceVK::SetData(sub_resources, target_cmd_queue);

    m_vk_copy_regions.clear();
    m_vk_copy_regions.reserve(sub_resources.size());

    const vk::DeviceMemory& vk_device_memory = m_vk_unique_staging_memory.get();
    for(const SubResource& sub_resource : sub_resources)
    {
        ValidateSubResource(sub_resource);

        // TODO: calculate memory offset by sub-resource index
        const vk::DeviceSize sub_resource_offset = 0U;
        Data::RawPtr sub_resource_data_ptr = nullptr;
        const vk::Result vk_map_result = GetNativeDevice().mapMemory(vk_device_memory, sub_resource_offset, sub_resource.GetDataSize(), vk::MemoryMapFlags{},
                                                                     reinterpret_cast<void**>(&sub_resource_data_ptr)); // NOSONAR

        META_CHECK_ARG_EQUAL_DESCR(vk_map_result, vk::Result::eSuccess, "failed to map staging buffer subresource");
        META_CHECK_ARG_NOT_NULL_DESCR(sub_resource_data_ptr, "failed to map buffer subresource");
        std::copy(sub_resource.GetDataPtr(), sub_resource.GetDataEndPtr(), sub_resource_data_ptr);

        GetNativeDevice().unmapMemory(vk_device_memory);

        m_vk_copy_regions.emplace_back(
            sub_resource_offset, 0, 0,
            vk::ImageSubresourceLayers(
                vk::ImageAspectFlagBits::eColor,
                sub_resource.GetIndex().GetMipLevel(),
                sub_resource.GetIndex().GetArrayIndex(),
                1U),
                vk::Offset3D(),
                TypeConverterVK::DimensionsToExtent3D(GetSettings().dimensions)
        );
    }

    // Copy buffer data from staging upload resource to the device-local GPU resource
    BlitCommandListVK& upload_cmd_list = PrepareResourceUpload(target_cmd_queue);
    upload_cmd_list.GetNativeCommandBufferDefault().copyBufferToImage(m_vk_unique_staging_buffer.get(), GetNativeResource(),
                                                                      vk::ImageLayout::eTransferDstOptimal, m_vk_copy_regions);

    if (GetSettings().mipmapped && sub_resources.size() < GetSubresourceCount().GetRawCount())
    {
        GenerateMipLevels();
    }

    CompleteResourceUpload(upload_cmd_list, State::ShaderResource, target_cmd_queue);
    GetContext().RequestDeferredAction(Context::DeferredAction::UploadResources);
}

bool ImageTextureVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!ResourceVK::SetName(name))
        return false;

    if (m_vk_unique_staging_buffer)
    {
        SetVulkanObjectName(GetNativeDevice(), m_vk_unique_staging_buffer.get(), fmt::format("{} Staging Buffer", name));
    }
    return true;
}

void ImageTextureVK::GenerateMipLevels()
{
    META_FUNCTION_TASK();
}

vk::ImageSubresourceRange ImageTextureVK::GetNativeSubresourceRange() const noexcept
{
    META_FUNCTION_TASK();
    const SubResource::Count& subresource_count = GetSubresourceCount();
    return vk::ImageSubresourceRange(
        vk::ImageAspectFlagBits::eColor,
        0U, subresource_count.GetMipLevelsCount(),
        0U, subresource_count.GetArraySize()
    );
}

} // namespace Methane::Graphics
