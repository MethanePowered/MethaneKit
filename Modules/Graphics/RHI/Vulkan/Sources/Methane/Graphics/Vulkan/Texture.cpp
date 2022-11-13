/******************************************************************************

Copyright 2019-2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/Texture.cpp
Vulkan implementation of the texture interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/Texture.h>
#include <Methane/Graphics/Vulkan/RenderContext.h>
#include <Methane/Graphics/Vulkan/RenderCommandList.h>
#include <Methane/Graphics/Vulkan/Device.h>
#include <Methane/Graphics/Vulkan/Types.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <algorithm>

namespace Methane::Graphics::Rhi
{

Ptr<ITexture> Rhi::ITexture::CreateRenderTarget(const Rhi::IRenderContext& render_context, const Settings& settings)
{
    META_FUNCTION_TASK();
    switch (settings.type)
    {
    case TextureType::Texture:            return std::make_shared<Vulkan::RenderTargetTexture>(dynamic_cast<const Vulkan::RenderContext&>(render_context), settings);
    case TextureType::DepthStencilBuffer: return std::make_shared<Vulkan::DepthStencilTexture>(dynamic_cast<const Vulkan::RenderContext&>(render_context), settings,
                                                                                            render_context.GetSettings().clear_depth_stencil);
    case TextureType::FrameBuffer: META_UNEXPECTED_ARG_DESCR(settings.type, "frame buffer texture must be created with static method Texture::CreateFrameBuffer");
    default:                         META_UNEXPECTED_ARG_RETURN(settings.type, nullptr);
    }

}

Ptr<ITexture> Rhi::ITexture::CreateFrameBuffer(const Rhi::IRenderContext& context, FrameBufferIndex frame_buffer_index)
{
    META_FUNCTION_TASK();
    const RenderContextSettings& context_settings = context.GetSettings();
    const Settings texture_settings = Settings::FrameBuffer(Dimensions(context_settings.frame_size), context_settings.color_format);
    return std::make_shared<Vulkan::FrameBufferTexture>(dynamic_cast<const Vulkan::RenderContext&>(context), texture_settings, frame_buffer_index);
}

Ptr<ITexture> Rhi::ITexture::CreateDepthStencilBuffer(const Rhi::IRenderContext& context)
{
    META_FUNCTION_TASK();
    const RenderContextSettings& context_settings = context.GetSettings();
    const Settings texture_settings = Settings::DepthStencilBuffer(Dimensions(context_settings.frame_size), context_settings.depth_stencil_format);
    return std::make_shared<Vulkan::DepthStencilTexture>(dynamic_cast<const Vulkan::RenderContext&>(context), texture_settings, context_settings.clear_depth_stencil);
}

Ptr<ITexture> Rhi::ITexture::CreateImage(const Rhi::IContext& context, const Dimensions& dimensions, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped)
{
    META_FUNCTION_TASK();
    const Settings texture_settings = Settings::Image(dimensions, array_length_opt, pixel_format, mipmapped, Usage::ShaderRead);
    return std::make_shared<Vulkan::ImageTexture>(dynamic_cast<const Vulkan::RenderContext&>(context), texture_settings);
}

Ptr<ITexture> Rhi::ITexture::CreateCube(const Rhi::IContext& context, uint32_t dimension_size, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped)
{
    META_FUNCTION_TASK();
    const Settings texture_settings = Settings::Cube(dimension_size, array_length_opt, pixel_format, mipmapped, Usage::ShaderRead);
    return std::make_shared<Vulkan::ImageTexture>(dynamic_cast<const Vulkan::RenderContext&>(context), texture_settings);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Vulkan
{

vk::ImageAspectFlags ITexture::GetNativeImageAspectFlags(const Rhi::TextureSettings& settings)
{
    META_FUNCTION_TASK();
    switch(settings.type)
    {
    case Rhi::TextureType::Texture:
    case Rhi::TextureType::FrameBuffer:        return vk::ImageAspectFlagBits::eColor;
    case Rhi::TextureType::DepthStencilBuffer: return IsDepthFormat(settings.pixel_format)
                                                    ? vk::ImageAspectFlagBits::eDepth
                                                    : vk::ImageAspectFlagBits::eStencil;
    default: META_UNEXPECTED_ARG_DESCR_RETURN(settings.type, vk::ImageAspectFlagBits::eColor, "Unsupported texture type");
    }
}

vk::ImageUsageFlags ITexture::GetNativeImageUsageFlags(const Rhi::TextureSettings& settings, vk::ImageUsageFlags initial_usage_flags)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;
    
    vk::ImageUsageFlags usage_flags = initial_usage_flags;
    switch (settings.type)
    {
    case Rhi::TextureType::FrameBuffer:
        usage_flags |= vk::ImageUsageFlagBits::eColorAttachment;
        break;

    case Rhi::TextureType::DepthStencilBuffer:
        usage_flags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
        break;

    case Rhi::TextureType::Texture:
        if (static_cast<bool>(settings.usage_mask & Rhi::ResourceUsage::RenderTarget))
            usage_flags |= vk::ImageUsageFlagBits::eColorAttachment;
        break;

    default:
        META_UNEXPECTED_ARG(settings.type);
    }
    
    if (settings.mipmapped)
    {
        // Flags required for mip-map generation with BLIT operations
        usage_flags |= vk::ImageUsageFlagBits::eTransferSrc;
        usage_flags |= vk::ImageUsageFlagBits::eTransferDst;
    }

    if (static_cast<bool>(settings.usage_mask & Rhi::ResourceUsage::ShaderRead))
        usage_flags |= vk::ImageUsageFlagBits::eSampled;

    return usage_flags;
}

static vk::ImageCreateFlags GetNativeImageCreateFlags(const Rhi::TextureSettings& settings)
{
    META_FUNCTION_TASK();
    vk::ImageCreateFlags image_create_flags{};
    switch (settings.dimension_type)
    {
    case Rhi::TextureDimensionType::Cube:
    case Rhi::TextureDimensionType::CubeArray:
        image_create_flags |= vk::ImageCreateFlagBits::eCubeCompatible;
        break;

    case Rhi::TextureDimensionType::Tex3D:
        image_create_flags |= vk::ImageCreateFlagBits::e2DArrayCompatible;
        break;

    default:
        break;
    }
    return image_create_flags;
}

static vk::UniqueImage CreateNativeImage(const IContext& context, const Rhi::TextureSettings& settings, vk::ImageUsageFlags initial_usage_flags = {})
{
    META_FUNCTION_TASK();
    return context.GetVulkanDevice().GetNativeDevice().createImageUnique(
        vk::ImageCreateInfo(
            GetNativeImageCreateFlags(settings),
            ITexture::DimensionTypeToImageType(settings.dimension_type),
            TypeConverter::PixelFormatToVulkan(settings.pixel_format),
            settings.dimension_type == Rhi::TextureDimensionType::Tex3D
                ? TypeConverter::DimensionsToExtent3D(settings.dimensions)
                : TypeConverter::FrameSizeToExtent3D(settings.dimensions.AsRectSize()),
            settings.mipmapped
                ? Base::Texture::GetRequiredMipLevelsCount(settings.dimensions)
                : 1U,
            settings.array_length * settings.dimensions.GetDepth(),
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            ITexture::GetNativeImageUsageFlags(settings, initial_usage_flags),
            vk::SharingMode::eExclusive));
}

static vk::ImageLayout GetVulkanImageLayoutByUsage(Rhi::TextureType texture_type, Rhi::ResourceUsage usage) noexcept
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;
    if (static_cast<bool>(usage & Rhi::ResourceUsage::ShaderRead))
    {
        return texture_type == Rhi::TextureType::DepthStencilBuffer
             ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
             : vk::ImageLayout::eShaderReadOnlyOptimal;
    }

    if (static_cast<bool>(usage & Rhi::ResourceUsage::ShaderWrite) ||
        static_cast<bool>(usage & Rhi::ResourceUsage::RenderTarget))
    {
        return texture_type == Rhi::TextureType::DepthStencilBuffer
             ? vk::ImageLayout::eDepthStencilAttachmentOptimal
             : vk::ImageLayout::eColorAttachmentOptimal;
    }

    return vk::ImageLayout::eUndefined;
}

static Ptr<ResourceView::ViewDescriptorVariant> CreateNativeImageViewDescriptor(const ResourceView::Id& view_id,
                                                                                const Rhi::TextureSettings& texture_settings,
                                                                                const Rhi::SubResource::Count& texture_subresource_count,
                                                                                const std::string& texture_name,
                                                                                const vk::Device& vk_device,
                                                                                const vk::Image& vk_image)
{
    META_FUNCTION_TASK();
    ResourceView::ImageViewDescriptor image_view_desc;

    image_view_desc.vk_view = vk_device.createImageViewUnique(
        vk::ImageViewCreateInfo(
            vk::ImageViewCreateFlags{},
            vk_image,
            ITexture::DimensionTypeToImageViewType(view_id.texture_dimension_type_opt.value_or(texture_settings.dimension_type)),
            TypeConverter::PixelFormatToVulkan(texture_settings.pixel_format),
            vk::ComponentMapping(),
            vk::ImageSubresourceRange(ITexture::GetNativeImageAspectFlags(texture_settings),
                                      view_id.subresource_index.GetMipLevel(),
                                      view_id.subresource_count.GetMipLevelsCount(),
                                      view_id.subresource_index.GetBaseLayerIndex(texture_subresource_count),
                                      view_id.subresource_count.GetBaseLayerCount())
        ));

    const std::string view_name = fmt::format("{} Image View for {} usage", texture_name, magic_enum::enum_name(view_id.usage));
    SetVulkanObjectName(vk_device, image_view_desc.vk_view.get(), view_name.c_str());

    image_view_desc.vk_desc = vk::DescriptorImageInfo(
        vk::Sampler(),
        *image_view_desc.vk_view,
        GetVulkanImageLayoutByUsage(texture_settings.type, view_id.usage)
    );

    return std::make_shared<ResourceView::ViewDescriptorVariant>(std::move(image_view_desc));
}

vk::ImageType ITexture::DimensionTypeToImageType(Rhi::TextureDimensionType dimension_type)
{
    META_FUNCTION_TASK();
    switch(dimension_type)
    {
    case Rhi::TextureDimensionType::Tex1D:
    case Rhi::TextureDimensionType::Tex1DArray:
        return vk::ImageType::e1D;

    case Rhi::TextureDimensionType::Tex2D:
    case Rhi::TextureDimensionType::Tex2DArray:
    case Rhi::TextureDimensionType::Tex2DMultisample:
    case Rhi::TextureDimensionType::Cube:
    case Rhi::TextureDimensionType::CubeArray:
        return vk::ImageType::e2D;

    case Rhi::TextureDimensionType::Tex3D:
        return vk::ImageType::e3D;

    default: META_UNEXPECTED_ARG_RETURN(dimension_type, vk::ImageType::e1D);
    }
}

vk::ImageViewType ITexture::DimensionTypeToImageViewType(Rhi::TextureDimensionType dimension_type)
{
    META_FUNCTION_TASK();
    switch(dimension_type)
    {
    case Rhi::TextureDimensionType::Tex1D:              return vk::ImageViewType::e1D;
    case Rhi::TextureDimensionType::Tex1DArray:         return vk::ImageViewType::e1DArray;
    case Rhi::TextureDimensionType::Tex2D:
    case Rhi::TextureDimensionType::Tex2DMultisample:   return vk::ImageViewType::e2D;
    case Rhi::TextureDimensionType::Tex2DArray:         return vk::ImageViewType::e2DArray;
    case Rhi::TextureDimensionType::Cube:               return vk::ImageViewType::eCube;
    case Rhi::TextureDimensionType::CubeArray:          return vk::ImageViewType::eCubeArray;
    case Rhi::TextureDimensionType::Tex3D:              return vk::ImageViewType::e3D;
    default: META_UNEXPECTED_ARG_RETURN(dimension_type, vk::ImageViewType::e1D);
    }
}

FrameBufferTexture::FrameBufferTexture(const RenderContext& render_context, const Settings& settings, FrameBufferIndex frame_buffer_index)
    : Resource(render_context, settings, render_context.GetNativeFrameImage(frame_buffer_index))
    , m_render_context(render_context)
    , m_frame_buffer_index(frame_buffer_index)
{
    META_FUNCTION_TASK();
}

void FrameBufferTexture::SetData(const SubResources&, Rhi::ICommandQueue&)
{
    META_FUNCTION_NOT_IMPLEMENTED_DESCR("frame-buffer textures do not support data setup");
}

vk::ImageSubresourceRange FrameBufferTexture::GetNativeSubresourceRange() const noexcept
{
    META_FUNCTION_TASK();
    return vk::ImageSubresourceRange(
        vk::ImageAspectFlagBits::eColor,
        0U, 1U,
        0U, 1U
    );
}

Ptr<ResourceView::ViewDescriptorVariant> FrameBufferTexture::CreateNativeViewDescriptor(const ResourceView::Id& view_id)
{
    META_FUNCTION_TASK();
    return CreateNativeImageViewDescriptor(view_id, GetSettings(), GetSubresourceCount(), GetName(), GetNativeDevice(), GetNativeImage());
}

void FrameBufferTexture::ResetNativeImage()
{
    META_FUNCTION_TASK();
    ResetNativeResource(m_render_context.GetNativeFrameImage(m_frame_buffer_index));
    ResetNativeViewDescriptors();
}

DepthStencilTexture::DepthStencilTexture(const RenderContext& render_context, const Settings& settings,
                                         const Opt<DepthStencil>& depth_stencil_opt)
    : Resource(render_context, settings, CreateNativeImage(render_context, settings))
    , m_depth_stencil_opt(depth_stencil_opt)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL_DESCR(settings.dimension_type, Rhi::TextureDimensionType::Tex2D, "depth-stencil texture is supported only with 2D dimensions");
    META_CHECK_ARG_EQUAL_DESCR(settings.dimensions.GetDepth(), 1U, "depth-stencil texture does not support 3D dimensions");
    META_CHECK_ARG_FALSE_DESCR(settings.mipmapped, "depth-stencil texture does not support mip-map mode");
    META_CHECK_ARG_EQUAL_DESCR(settings.array_length, 1U, "depth-stencil texture does not support arrays");

    // Allocate resource primary memory
    const vk::Device& vk_device = GetNativeDevice();
    AllocateResourceMemory(vk_device.getImageMemoryRequirements(GetNativeResource()), vk::MemoryPropertyFlagBits::eDeviceLocal);
    vk_device.bindImageMemory(GetNativeResource(), GetNativeDeviceMemory(), 0);
}

void DepthStencilTexture::SetData(const SubResources&, Rhi::ICommandQueue&)
{
    META_FUNCTION_NOT_IMPLEMENTED_DESCR("depth-stencil textures do not support data setup");
}

vk::ImageSubresourceRange DepthStencilTexture::GetNativeSubresourceRange() const noexcept
{
    META_FUNCTION_TASK();
    return vk::ImageSubresourceRange(
        vk::ImageAspectFlagBits::eDepth,
        0U, 1U,
        0U, 1U
    );
}

Ptr<ResourceView::ViewDescriptorVariant> DepthStencilTexture::CreateNativeViewDescriptor(const ResourceView::Id& view_id)
{
    META_FUNCTION_TASK();
    return CreateNativeImageViewDescriptor(view_id, GetSettings(), GetSubresourceCount(), GetName(), GetNativeDevice(), GetNativeImage());
}

RenderTargetTexture::RenderTargetTexture(const RenderContext& render_context, const Settings& settings)
    : Resource(render_context, settings, CreateNativeImage(render_context, settings))
{
    META_FUNCTION_TASK();
    // Allocate resource primary memory
    const vk::Device& vk_device = GetNativeDevice();
    AllocateResourceMemory(vk_device.getImageMemoryRequirements(GetNativeResource()), vk::MemoryPropertyFlagBits::eDeviceLocal);
    vk_device.bindImageMemory(GetNativeResource(), GetNativeDeviceMemory(), 0);
}

void RenderTargetTexture::SetData(const SubResources&, Rhi::ICommandQueue&)
{
    META_FUNCTION_NOT_IMPLEMENTED_DESCR("render-target textures do not support data setup");
}

vk::ImageSubresourceRange RenderTargetTexture::GetNativeSubresourceRange() const noexcept
{
    META_FUNCTION_TASK();
    const SubResource::Count& subresource_count = GetSubresourceCount();
    return vk::ImageSubresourceRange(
        vk::ImageAspectFlagBits::eColor,
        0U, subresource_count.GetMipLevelsCount(),
        0U, subresource_count.GetBaseLayerCount()
    );
}

Ptr<ResourceView::ViewDescriptorVariant> RenderTargetTexture::CreateNativeViewDescriptor(const ResourceView::Id& view_id)
{
    META_FUNCTION_TASK();
    return CreateNativeImageViewDescriptor(view_id, GetSettings(), GetSubresourceCount(), GetName(), GetNativeDevice(), GetNativeImage());
}

ImageTexture::ImageTexture(const Base::Context& context, const Settings& settings)
    : Resource(context, settings, CreateNativeImage(dynamic_cast<const IContext&>(context), settings, vk::ImageUsageFlagBits::eTransferDst))
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

void ImageTexture::SetData(const SubResources& sub_resources, Rhi::ICommandQueue& target_cmd_queue)
{
    META_FUNCTION_TASK();
    Resource::SetData(sub_resources, target_cmd_queue);

    m_vk_copy_regions.clear();
    m_vk_copy_regions.reserve(sub_resources.size());

    const SubResource::Count& subresource_count = GetSubresourceCount();
    const vk::DeviceMemory& vk_device_memory = m_vk_unique_staging_memory.get();
    vk::DeviceSize sub_resource_offset = 0U;

    for(const SubResource& sub_resource : sub_resources)
    {
        ValidateSubResource(sub_resource);
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
                sub_resource.GetIndex().GetBaseLayerIndex(subresource_count),
                1U
            ),
            vk::Offset3D(),
            TypeConverter::FrameSizeToExtent3D(GetSettings().dimensions.AsRectSize())
        );

        sub_resource_offset += sub_resource.GetDataSize();
    }

    // Copy buffer data from staging upload resource to the device-local GPU resource
    TransferCommandList&   upload_cmd_list = PrepareResourceUpload(target_cmd_queue);
    const vk::CommandBuffer& vk_cmd_buffer = upload_cmd_list.GetNativeCommandBufferDefault();
    vk_cmd_buffer.copyBufferToImage(m_vk_unique_staging_buffer.get(), GetNativeResource(),
                                    vk::ImageLayout::eTransferDstOptimal, m_vk_copy_regions);

    if (GetSettings().mipmapped && sub_resources.size() < GetSubresourceCount().GetRawCount())
    {
        CompleteResourceUpload(upload_cmd_list, GetState(), target_cmd_queue); // ownership transition only
        GenerateMipLevels(target_cmd_queue, State::ShaderResource);
    }
    else
    {
        CompleteResourceUpload(upload_cmd_list, State::ShaderResource, target_cmd_queue);
    }
    GetContext().RequestDeferredAction(Rhi::IContext::DeferredAction::UploadResources);
}

bool ImageTexture::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!Resource::SetName(name))
        return false;

    if (m_vk_unique_staging_buffer)
    {
        SetVulkanObjectName(GetNativeDevice(), m_vk_unique_staging_buffer.get(), fmt::format("{} Staging Buffer", name));
    }
    return true;
}

void ImageTexture::GenerateMipLevels(Rhi::ICommandQueue& target_cmd_queue, State target_resource_state)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL_DESCR(target_cmd_queue.GetCommandListType(), Rhi::CommandListType::Render,
                               "texture target command queue is not suitable for mip-maps generation");

    const Rhi::TextureSettings& texture_settings = GetSettings();
    const vk::Format image_format = TypeConverter::PixelFormatToVulkan(texture_settings.pixel_format);
    const vk::FormatProperties image_format_properties = GetVulkanContext().GetVulkanDevice().GetNativePhysicalDevice().getFormatProperties(image_format);
    META_CHECK_ARG_TRUE_DESCR(static_cast<bool>(image_format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear),
                              "texture pixel format does not support linear blitting");

    constexpr auto post_upload_cmd_list_id = static_cast<Rhi::CommandListId>(Rhi::CommandListPurpose::PostUploadSync);
    const Rhi::ICommandList     & target_cmd_list = GetContext().GetDefaultCommandKit(target_cmd_queue).GetListForEncoding(post_upload_cmd_list_id);
    const vk::CommandBuffer& vk_cmd_buffer   = dynamic_cast<const RenderCommandList&>(target_cmd_list).GetNativeCommandBufferDefault();

    const SubResource::Count& subresource_count = GetSubresourceCount();
    const uint32_t mip_levels_count = subresource_count.GetMipLevelsCount();
    const State source_resource_state = GetState();

    const vk::ImageLayout        vk_old_image_layout = IResource::GetNativeImageLayoutByResourceState(source_resource_state);
    const vk::AccessFlags        vk_src_access_mask  = IResource::GetNativeAccessFlagsByResourceState(source_resource_state);
    const vk::PipelineStageFlags vk_src_stage_mask   = IResource::GetNativePipelineStageFlagsByResourceState(source_resource_state);

    const vk::ImageLayout        vk_new_image_layout = IResource::GetNativeImageLayoutByResourceState(target_resource_state);
    const vk::AccessFlags        vk_dst_access_mask  = IResource::GetNativeAccessFlagsByResourceState(target_resource_state);
    const vk::PipelineStageFlags vk_dst_stage_mask   = IResource::GetNativePipelineStageFlagsByResourceState(target_resource_state);

    const vk::ImageLayout   vk_blit_old_image_layout = vk::ImageLayout::eTransferSrcOptimal;
    const vk::ImageLayout   vk_blit_new_image_layout = vk::ImageLayout::eTransferDstOptimal;
    const vk::AccessFlags   vk_blit_src_access_mask  = vk::AccessFlagBits::eTransferRead;

    const vk::Image& vk_image = GetNativeImage();

    for(uint32_t base_layer_index = 0U; base_layer_index < subresource_count.GetBaseLayerCount(); ++base_layer_index)
    {
        vk::ImageMemoryBarrier vk_image_barrier;
        vk_image_barrier.image                           = vk_image;
        vk_image_barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        vk_image_barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        vk_image_barrier.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
        vk_image_barrier.subresourceRange.baseArrayLayer = base_layer_index;
        vk_image_barrier.subresourceRange.layerCount     = 1U;
        vk_image_barrier.subresourceRange.levelCount     = 1U;

        auto prev_mip_width  = static_cast<int32_t>(texture_settings.dimensions.GetWidth());
        auto prev_mip_height = static_cast<int32_t>(texture_settings.dimensions.GetHeight());

        for (uint32_t mip_level_index = 1U; mip_level_index < mip_levels_count; ++mip_level_index)
        {
            const int32_t  curr_mip_width       = prev_mip_width > 1 ? prev_mip_width / 2 : 1;
            const int32_t  curr_mip_height      = prev_mip_height > 1 ? prev_mip_height / 2 : 1;
            const uint32_t prev_mip_level_index = mip_level_index - 1U;

            vk_image_barrier.subresourceRange.baseMipLevel = prev_mip_level_index;
            vk_image_barrier.oldLayout                     = vk_old_image_layout;
            vk_image_barrier.newLayout                     = vk_blit_old_image_layout;
            vk_image_barrier.srcAccessMask                 = vk_src_access_mask;
            vk_image_barrier.dstAccessMask                 = vk_blit_src_access_mask;

            vk_cmd_buffer.pipelineBarrier(
                vk_src_stage_mask,
                vk_src_stage_mask,
                vk::DependencyFlags{},
                0U, nullptr,
                0U, nullptr,
                1U, &vk_image_barrier
            );

            const vk::ImageBlit vk_image_blit(
                vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, prev_mip_level_index, base_layer_index, 1U),
                { vk::Offset3D(), vk::Offset3D(prev_mip_width, prev_mip_height, 1U) },
                vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, mip_level_index, base_layer_index, 1U),
                { vk::Offset3D(), vk::Offset3D(curr_mip_width, curr_mip_height, 1U) }
            );

            vk_cmd_buffer.blitImage(
                vk_image, vk_blit_old_image_layout,
                vk_image, vk_blit_new_image_layout,
                1U, &vk_image_blit,
                vk::Filter::eLinear
            );

            vk_image_barrier.oldLayout     = vk_blit_old_image_layout;
            vk_image_barrier.newLayout     = vk_new_image_layout;
            vk_image_barrier.srcAccessMask = vk_blit_src_access_mask;
            vk_image_barrier.dstAccessMask = vk_dst_access_mask;

            vk_cmd_buffer.pipelineBarrier(
                vk_src_stage_mask,
                vk_dst_stage_mask,
                vk::DependencyFlags{},
                0U, nullptr,
                0U, nullptr,
                1U, &vk_image_barrier
            );

            prev_mip_width  = curr_mip_width;
            prev_mip_height = curr_mip_height;
        }

        vk_image_barrier.subresourceRange.baseMipLevel = mip_levels_count - 1U;
        vk_image_barrier.oldLayout                     = vk_blit_new_image_layout;
        vk_image_barrier.newLayout                     = vk_new_image_layout;
        vk_image_barrier.srcAccessMask                 = vk_blit_src_access_mask;
        vk_image_barrier.dstAccessMask                 = vk_dst_access_mask;

        vk_cmd_buffer.pipelineBarrier(
            vk_src_stage_mask,
            vk_dst_stage_mask,
            vk::DependencyFlags{},
            0U, nullptr,
            0U, nullptr,
            1U, &vk_image_barrier);
    }

    SetState(target_resource_state);
}

vk::ImageSubresourceRange ImageTexture::GetNativeSubresourceRange() const noexcept
{
    META_FUNCTION_TASK();
    const SubResource::Count& subresource_count = GetSubresourceCount();
    return vk::ImageSubresourceRange(
        vk::ImageAspectFlagBits::eColor,
        0U, subresource_count.GetMipLevelsCount(),
        0U, subresource_count.GetBaseLayerCount()
    );
}

Ptr<ResourceView::ViewDescriptorVariant> ImageTexture::CreateNativeViewDescriptor(const ResourceView::Id& view_id)
{
    META_FUNCTION_TASK();
    return CreateNativeImageViewDescriptor(view_id, GetSettings(), GetSubresourceCount(), GetName(), GetNativeDevice(), GetNativeImage());
}

} // namespace Methane::Graphics::Vulkan
