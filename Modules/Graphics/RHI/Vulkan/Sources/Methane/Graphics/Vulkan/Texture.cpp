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

#include <Methane/Data/EnumMaskUtil.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <algorithm>

namespace Methane::Graphics::Vulkan
{

vk::ImageAspectFlags Texture::GetNativeImageAspectFlags(const Rhi::TextureSettings& settings)
{
    META_FUNCTION_TASK();
    switch(settings.type)
    {
    case Rhi::TextureType::Image:
    case Rhi::TextureType::RenderTarget:
    case Rhi::TextureType::FrameBuffer:  return vk::ImageAspectFlagBits::eColor;
    case Rhi::TextureType::DepthStencil: return IsDepthFormat(settings.pixel_format)
                                              ? vk::ImageAspectFlagBits::eDepth
                                              : vk::ImageAspectFlagBits::eStencil;
    default: META_UNEXPECTED_ARG_DESCR_RETURN(settings.type, vk::ImageAspectFlagBits::eColor, "Unsupported texture type");
    }
}

vk::ImageUsageFlags Texture::GetNativeImageUsageFlags(const Rhi::TextureSettings& settings, vk::ImageUsageFlags initial_usage_flags)
{
    META_FUNCTION_TASK();
    vk::ImageUsageFlags usage_flags = initial_usage_flags;

    switch (settings.type)
    {
    case Rhi::TextureType::FrameBuffer:
        usage_flags |= vk::ImageUsageFlagBits::eColorAttachment;
        break;

    case Rhi::TextureType::DepthStencil:
        usage_flags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
        break;

    case Rhi::TextureType::Image:
        if (settings.usage_mask.HasAnyBit(Rhi::ResourceUsage::ShaderWrite))
            usage_flags |= vk::ImageUsageFlagBits::eStorage;
        [[fallthrough]];

    case Rhi::TextureType::RenderTarget:
        if (settings.usage_mask.HasAnyBit(Rhi::ResourceUsage::RenderTarget))
            usage_flags |= vk::ImageUsageFlagBits::eColorAttachment;
        break;
    }
    
    if (settings.mipmapped)
    {
        // Flags required for mip-map generation with BLIT operations
        usage_flags |= vk::ImageUsageFlagBits::eTransferSrc;
        usage_flags |= vk::ImageUsageFlagBits::eTransferDst;
    }

    if (settings.usage_mask.HasAnyBit(Rhi::ResourceUsage::ShaderRead))
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
            Texture::DimensionTypeToImageType(settings.dimension_type),
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
            Texture::GetNativeImageUsageFlags(settings, initial_usage_flags),
            vk::SharingMode::eExclusive));
}

static vk::ImageLayout GetVulkanImageLayoutByUsage(Rhi::TextureType texture_type, Rhi::ResourceUsageMask usage) noexcept
{
    META_FUNCTION_TASK();
    if (usage.HasAnyBit(Rhi::ResourceUsage::ShaderWrite))
    {
        return vk::ImageLayout::eGeneral;
    }
    if (usage.HasAnyBit(Rhi::ResourceUsage::ShaderRead))
    {
        return texture_type == Rhi::TextureType::DepthStencil
             ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
             : vk::ImageLayout::eShaderReadOnlyOptimal;
    }
    if (usage.HasAnyBit(Rhi::ResourceUsage::RenderTarget))
    {
        return texture_type == Rhi::TextureType::DepthStencil
             ? vk::ImageLayout::eDepthStencilAttachmentOptimal
             : vk::ImageLayout::eColorAttachmentOptimal;
    }
    return vk::ImageLayout::eUndefined;
}

static Ptr<ResourceView::ViewDescriptorVariant> CreateNativeImageViewDescriptor(const ResourceView::Id& view_id,
                                                                                const Rhi::TextureSettings& texture_settings,
                                                                                const Rhi::SubResource::Count& texture_subresource_count,
                                                                                std::string_view texture_name,
                                                                                const vk::Device& vk_device,
                                                                                const vk::Image& vk_image)
{
    META_FUNCTION_TASK();
    ResourceView::ImageViewDescriptor image_view_desc;

    image_view_desc.vk_view = vk_device.createImageViewUnique(
        vk::ImageViewCreateInfo(
            vk::ImageViewCreateFlags{},
            vk_image,
            Texture::DimensionTypeToImageViewType(view_id.texture_dimension_type_opt.value_or(texture_settings.dimension_type)),
            TypeConverter::PixelFormatToVulkan(texture_settings.pixel_format),
            vk::ComponentMapping(),
            vk::ImageSubresourceRange(Texture::GetNativeImageAspectFlags(texture_settings),
                                      view_id.subresource_index.GetMipLevel(),
                                      view_id.subresource_count.GetMipLevelsCount(),
                                      view_id.subresource_index.GetBaseLayerIndex(texture_subresource_count),
                                      view_id.subresource_count.GetBaseLayerCount())
        ));

    const std::string view_name = fmt::format("{} Image View for {} usage", texture_name, Data::GetEnumMaskName(view_id.usage));
    SetVulkanObjectName(vk_device, image_view_desc.vk_view.get(), view_name.c_str());

    image_view_desc.vk_desc = vk::DescriptorImageInfo(
        vk::Sampler(),
        *image_view_desc.vk_view,
        GetVulkanImageLayoutByUsage(texture_settings.type, view_id.usage)
    );

    return std::make_shared<ResourceView::ViewDescriptorVariant>(std::move(image_view_desc));
}

vk::ImageType Texture::DimensionTypeToImageType(Rhi::TextureDimensionType dimension_type)
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

vk::ImageViewType Texture::DimensionTypeToImageViewType(Rhi::TextureDimensionType dimension_type)
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

Texture::Texture(const Base::Context& context, const Settings& settings)
    : Texture(context, settings, CreateNativeImage(dynamic_cast<const IContext&>(context), settings,
                                                   vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc))
{
}

Texture::Texture(const RenderContext& render_context, const Settings& settings, Data::Index frame_index)
    : Resource(render_context, settings, render_context.GetNativeFrameImage(frame_index))
{
    META_CHECK_ARG_TRUE(settings.frame_index_opt.has_value());
    META_CHECK_ARG_EQUAL(frame_index, settings.frame_index_opt.value());
}

Texture::Texture(const Base::Context& context, const Settings& settings, vk::UniqueImage&& vk_unique_image)
    : Resource(context, settings, vk_unique_image.get())
    , m_vk_unique_image(std::move(vk_unique_image))
{
    META_FUNCTION_TASK();
    switch(settings.type)
    {
    case Rhi::TextureType::Image:        InitializeAsImage(); break;
    case Rhi::TextureType::RenderTarget: InitializeAsRenderTarget(); break;
    case Rhi::TextureType::DepthStencil: InitializeAsDepthStencil(); break;
    //   Rhi::TextureType::FrameBuffer:  initialized with a separate constructor
    default:                             META_UNEXPECTED_ARG(settings.type);
    }

}

void Texture::InitializeAsImage()
{
    META_FUNCTION_TASK();
    const Settings& settings = GetSettings();
    META_CHECK_ARG_EQUAL(settings.type, Rhi::TextureType::Image);

    // Allocate resource primary memory
    const vk::Device& vk_device = GetNativeDevice();
    const vk::MemoryRequirements vk_image_memory_requirements = vk_device.getImageMemoryRequirements(GetNativeResource());
    AllocateResourceMemory(vk_image_memory_requirements, vk::MemoryPropertyFlagBits::eDeviceLocal);
    vk_device.bindImageMemory(GetNativeResource(), GetNativeDeviceMemory(), 0);

    // Create staging buffer and allocate staging memory
    m_vk_unique_staging_buffer = vk_device.createBufferUnique(
        vk::BufferCreateInfo(vk::BufferCreateFlags{},
                             vk_image_memory_requirements.size,
                             vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
                             vk::SharingMode::eExclusive)
    );

    const vk::MemoryPropertyFlags vk_staging_memory_flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    m_vk_unique_staging_memory = AllocateDeviceMemory(vk_device.getBufferMemoryRequirements(m_vk_unique_staging_buffer.get()), vk_staging_memory_flags);
    vk_device.bindBufferMemory(m_vk_unique_staging_buffer.get(), m_vk_unique_staging_memory.get(), 0);
}

void Texture::InitializeAsRenderTarget()
{
    META_FUNCTION_TASK();
    const Settings& settings = GetSettings();
    META_CHECK_ARG_EQUAL(settings.type, Rhi::TextureType::RenderTarget);

    // Allocate resource primary memory
    const vk::Device& vk_device = GetNativeDevice();
    AllocateResourceMemory(vk_device.getImageMemoryRequirements(GetNativeResource()), vk::MemoryPropertyFlagBits::eDeviceLocal);
    vk_device.bindImageMemory(GetNativeResource(), GetNativeDeviceMemory(), 0);
}

void Texture::InitializeAsDepthStencil()
{
    META_FUNCTION_TASK();
    const Settings& settings = GetSettings();
    META_CHECK_ARG_EQUAL(settings.type, Rhi::TextureType::DepthStencil);

    META_CHECK_ARG_EQUAL_DESCR(settings.dimension_type, Rhi::TextureDimensionType::Tex2D, "depth-stencil texture is supported only with 2D dimensions");
    META_CHECK_ARG_EQUAL_DESCR(settings.dimensions.GetDepth(), 1U, "depth-stencil texture does not support 3D dimensions");
    META_CHECK_ARG_FALSE_DESCR(settings.mipmapped, "depth-stencil texture does not support mip-map mode");
    META_CHECK_ARG_EQUAL_DESCR(settings.array_length, 1U, "depth-stencil texture does not support arrays");

    // Allocate resource primary memory
    const vk::Device& vk_device = GetNativeDevice();
    AllocateResourceMemory(vk_device.getImageMemoryRequirements(GetNativeResource()), vk::MemoryPropertyFlagBits::eDeviceLocal);
    vk_device.bindImageMemory(GetNativeResource(), GetNativeDeviceMemory(), 0);
}

void Texture::ResetNativeFrameImage()
{
    META_FUNCTION_TASK();
    const Settings& settings = GetSettings();
    META_CHECK_ARG_EQUAL(settings.type, Rhi::TextureType::FrameBuffer);
    META_CHECK_ARG_TRUE(settings.frame_index_opt.has_value());

    const Base::Context& context = GetBaseContext();
    META_CHECK_ARG_EQUAL(context.GetType(), Rhi::ContextType::Render);

    const auto& render_context = static_cast<const RenderContext&>(context);
    ResetNativeResource(render_context.GetNativeFrameImage(settings.frame_index_opt.value()));
    ResetNativeViewDescriptors();
}

void Texture::SetData(Rhi::ICommandQueue& target_cmd_queue, const SubResources& sub_resources)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL_DESCR(GetSettings().type, Rhi::TextureType::Image, "only image textures support data upload from CPU");

    Base::Texture::SetData(target_cmd_queue, sub_resources);

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
    TransferCommandList&   upload_cmd_list = PrepareResourceTransfer(target_cmd_queue, State::CopyDest);
    const vk::CommandBuffer& vk_cmd_buffer = upload_cmd_list.GetNativeCommandBufferDefault();
    vk_cmd_buffer.copyBufferToImage(m_vk_unique_staging_buffer.get(), GetNativeResource(),
                                    vk::ImageLayout::eTransferDstOptimal, m_vk_copy_regions);

    if (GetSettings().mipmapped && sub_resources.size() < GetSubresourceCount().GetRawCount())
    {
        CompleteResourceTransfer(upload_cmd_list, GetState(), target_cmd_queue); // ownership transition only
        GenerateMipLevels(target_cmd_queue, State::ShaderResource);
    }
    else
    {
        CompleteResourceTransfer(upload_cmd_list, State::ShaderResource, target_cmd_queue);
    }
    GetContext().RequestDeferredAction(Rhi::IContext::DeferredAction::UploadResources);
}

Rhi::SubResource Texture::GetData(Rhi::ICommandQueue& target_cmd_queue, const SubResource::Index& sub_resource_index, const BytesRangeOpt& data_range)
{
    META_CHECK_ARG_EQUAL_DESCR(GetSettings().type, Rhi::TextureType::Image, "only image textures support data read-back from CPU");
    META_CHECK_ARG_TRUE_DESCR(GetUsage().HasAnyBit(Rhi::ResourceUsage::ReadBack),
                              "getting texture data from GPU is allowed for buffers with CPU Read-back flag only");

    const Settings&           settings          = GetSettings();
    const uint32_t            bytes_per_row     = settings.dimensions.GetWidth()  * GetPixelSize(settings.pixel_format);
    const uint32_t            bytes_per_image   = settings.dimensions.GetHeight() * bytes_per_row;
    const SubResource::Count& subresource_count = GetSubresourceCount();
    const State           initial_texture_state = GetState();

    // Copy texture data from device-local GPU resource to staging resource for read-back
    vk::BufferImageCopy image_to_buffer_copy(
        0U, 0U, 0U,
        vk::ImageSubresourceLayers(
            Texture::GetNativeImageAspectFlags(settings),
            sub_resource_index.GetMipLevel(),
            sub_resource_index.GetBaseLayerIndex(subresource_count),
            1U
        ),
        vk::Offset3D(),
        TypeConverter::FrameSizeToExtent3D(GetSettings().dimensions.AsRectSize())
    );
    TransferCommandList&   upload_cmd_list = PrepareResourceTransfer(target_cmd_queue, State::CopySource);
    const vk::CommandBuffer& vk_cmd_buffer = upload_cmd_list.GetNativeCommandBufferDefault();
    vk_cmd_buffer.copyImageToBuffer(GetNativeResource(), vk::ImageLayout::eTransferSrcOptimal,
                                    m_vk_unique_staging_buffer.get(), image_to_buffer_copy);

    CompleteResourceTransfer(upload_cmd_list, initial_texture_state, target_cmd_queue);

    // Execute resource transfer commands and wait for completion
    GetBaseContext().UploadResources();

    // Map staging buffer memory and copy texture subresource data
    const vk::DeviceMemory& vk_device_memory = m_vk_unique_staging_memory.get();
    Data::Size   staging_data_offset = 0U;
    Data::Size   staging_data_size   = bytes_per_image;
    Data::RawPtr staging_data_ptr    = nullptr;
    if (data_range)
    {
        META_CHECK_ARG_LESS_DESCR(data_range->GetEnd(), staging_data_size, "provided texture subresource data range is out of bounds");
        staging_data_offset = data_range->GetStart();
        staging_data_size   = data_range->GetLength();
    }
    const vk::Result vk_map_result = GetNativeDevice().mapMemory(vk_device_memory, staging_data_offset, staging_data_size, vk::MemoryMapFlags{},
                                                                 reinterpret_cast<void**>(&staging_data_ptr)); // NOSONAR

    META_CHECK_ARG_EQUAL_DESCR(vk_map_result, vk::Result::eSuccess, "failed to map staging buffer subresource");
    META_CHECK_ARG_NOT_NULL_DESCR(staging_data_ptr, "failed to map buffer subresource");
    Rhi::SubResource sub_resource(Data::Bytes(staging_data_ptr, staging_data_ptr + staging_data_size), sub_resource_index, data_range);

    GetNativeDevice().unmapMemory(vk_device_memory);
    return sub_resource;
}

bool Texture::SetName(std::string_view name)
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

void Texture::GenerateMipLevels(Rhi::ICommandQueue& target_cmd_queue, State target_resource_state)
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

vk::ImageSubresourceRange Texture::GetNativeSubresourceRange() const
{
    META_FUNCTION_TASK();
    const SubResource::Count& subresource_count = GetSubresourceCount();
    return vk::ImageSubresourceRange(
        Texture::GetNativeImageAspectFlags(GetSettings()),
        0U, subresource_count.GetMipLevelsCount(),
        0U, subresource_count.GetBaseLayerCount()
    );
}

Ptr<ResourceView::ViewDescriptorVariant> Texture::CreateNativeViewDescriptor(const ResourceView::Id& view_id)
{
    META_FUNCTION_TASK();
    return CreateNativeImageViewDescriptor(view_id, GetSettings(), GetSubresourceCount(), GetName(), GetNativeDevice(), GetNativeImage());
}

} // namespace Methane::Graphics::Vulkan
