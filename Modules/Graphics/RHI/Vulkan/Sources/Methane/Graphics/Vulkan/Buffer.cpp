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

FILE: Methane/Graphics/Vulkan/Buffer.cpp
Vulkan implementation of the buffer interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/Buffer.h>
#include <Methane/Graphics/Vulkan/IContext.h>

#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Base/Context.h>
#include <Methane/Instrumentation.h>

#include <iterator>

namespace Methane::Graphics::Vulkan
{

static vk::BufferUsageFlags GetVulkanBufferUsageFlags(Rhi::BufferType buffer_type, Rhi::BufferStorageMode storage_mode)
{
    META_FUNCTION_TASK();
    vk::BufferUsageFlags vk_usage_flags;
    switch(buffer_type)
    {
    case Rhi::BufferType::Storage:  vk_usage_flags |= vk::BufferUsageFlagBits::eStorageBuffer; break;
    case Rhi::BufferType::Constant: vk_usage_flags |= vk::BufferUsageFlagBits::eUniformBuffer; break;
    case Rhi::BufferType::Index:    vk_usage_flags |= vk::BufferUsageFlagBits::eIndexBuffer;   break;
    case Rhi::BufferType::Vertex:   vk_usage_flags |= vk::BufferUsageFlagBits::eVertexBuffer;  break;
    // Buffer::Type::ReadBack - unsupported
    default: META_UNEXPECTED_ARG_DESCR(buffer_type, "Unsupported buffer type");
    }

    if (storage_mode == Rhi::BufferStorageMode::Private)
        vk_usage_flags |= vk::BufferUsageFlagBits::eTransferDst;

    return vk_usage_flags;
}

static Rhi::ResourceState GetTargetResourceStateByBufferType(Rhi::BufferType buffer_type)
{
    META_FUNCTION_TASK();
    switch(buffer_type)
    {
    case Rhi::BufferType::Storage:     return Rhi::ResourceState::ShaderResource;
    case Rhi::BufferType::Constant:    return Rhi::ResourceState::ConstantBuffer;
    case Rhi::BufferType::Index:       return Rhi::ResourceState::IndexBuffer;
    case Rhi::BufferType::Vertex:      return Rhi::ResourceState::VertexBuffer;
    case Rhi::BufferType::ReadBack:    return Rhi::ResourceState::StreamOut;
    default: META_UNEXPECTED_ARG_DESCR_RETURN(buffer_type, Rhi::ResourceState::Undefined, "Unsupported buffer type");
    }
}

Buffer::Buffer(const Base::Context& context, const Settings& settings)
    : Resource(context, settings,
                 dynamic_cast<const IContext&>(context).GetVulkanDevice().GetNativeDevice().createBufferUnique(
                     vk::BufferCreateInfo(
                         vk::BufferCreateFlags{},
                         settings.size,
                         GetVulkanBufferUsageFlags(settings.type, settings.storage_mode),
                         vk::SharingMode::eExclusive)))
{
    META_FUNCTION_TASK();
    const bool is_private_storage = settings.storage_mode == Rhi::BufferStorageMode::Private;
    const vk::MemoryPropertyFlags vk_staging_memory_flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    const vk::MemoryPropertyFlags vk_memory_property_flags = is_private_storage ? vk::MemoryPropertyFlagBits::eDeviceLocal : vk_staging_memory_flags;

    // Allocate resource primary memory
    AllocateResourceMemory(GetNativeDevice().getBufferMemoryRequirements(GetNativeResource()), vk_memory_property_flags);
    GetNativeDevice().bindBufferMemory(GetNativeResource(), GetNativeDeviceMemory(), 0);

    if (!is_private_storage)
        return;

    // Create staging buffer and allocate staging memory
    m_vk_unique_staging_buffer = GetNativeDevice().createBufferUnique(
        vk::BufferCreateInfo(vk::BufferCreateFlags{},
            settings.size,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::SharingMode::eExclusive)
    );

    m_vk_unique_staging_memory = AllocateDeviceMemory(GetNativeDevice().getBufferMemoryRequirements(m_vk_unique_staging_buffer.get()), vk_staging_memory_flags);
    GetNativeDevice().bindBufferMemory(m_vk_unique_staging_buffer.get(), m_vk_unique_staging_memory.get(), 0);
}

void Buffer::SetData(const Rhi::SubResources& sub_resources, Rhi::ICommandQueue& target_cmd_queue)
{
    META_FUNCTION_TASK();
    Resource::SetData(sub_resources, target_cmd_queue);

    const Settings& buffer_settings = GetSettings();
    const bool is_private_storage = buffer_settings.storage_mode == Rhi::IBuffer::StorageMode::Private;
    if (is_private_storage)
    {
        m_vk_copy_regions.clear();
        m_vk_copy_regions.reserve(sub_resources.size());
    }

    const vk::DeviceMemory& vk_device_memory = is_private_storage ? m_vk_unique_staging_memory.get() : GetNativeDeviceMemory();
    for(const SubResource& sub_resource : sub_resources)
    {
        ValidateSubResource(sub_resource);

        const vk::DeviceSize sub_resource_offset = 0U;
        Data::RawPtr sub_resource_data_ptr = nullptr;
        const vk::Result vk_map_result = GetNativeDevice().mapMemory(vk_device_memory, sub_resource_offset, sub_resource.GetDataSize(), vk::MemoryMapFlags{},
                                                                     reinterpret_cast<void**>(&sub_resource_data_ptr)); // NOSONAR

        META_CHECK_ARG_EQUAL_DESCR(vk_map_result, vk::Result::eSuccess, "failed to map buffer subresource");
        META_CHECK_ARG_NOT_NULL_DESCR(sub_resource_data_ptr, "failed to map buffer subresource");
        std::copy(sub_resource.GetDataPtr(), sub_resource.GetDataEndPtr(), sub_resource_data_ptr);

        GetNativeDevice().unmapMemory(vk_device_memory);

        if (is_private_storage)
        {
            m_vk_copy_regions.emplace_back(sub_resource_offset, sub_resource_offset, static_cast<vk::DeviceSize>(sub_resource.GetDataSize()));
        }
    }

    if (!is_private_storage)
        return;

    // In case of private GPU storage, copy buffer data from staging upload resource to the device-local GPU resource
    TransferCommandList& upload_cmd_list = PrepareResourceTransfer(target_cmd_queue, State::CopyDest);
    upload_cmd_list.GetNativeCommandBufferDefault().copyBuffer(m_vk_unique_staging_buffer.get(), GetNativeResource(), m_vk_copy_regions);
    CompleteResourceTransfer(upload_cmd_list, GetTargetResourceStateByBufferType(buffer_settings.type), target_cmd_queue);
    GetContext().RequestDeferredAction(Rhi::ContextDeferredAction::UploadResources);
}

bool Buffer::SetName(std::string_view name)
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

Ptr<ResourceView::ViewDescriptorVariant> Buffer::CreateNativeViewDescriptor(const ResourceView::Id& view_id)
{
    META_FUNCTION_TASK();
    ResourceView::BufferViewDescriptor buffer_view_desc;
    buffer_view_desc.vk_desc = vk::DescriptorBufferInfo(
        GetNativeResource(),
        static_cast<vk::DeviceSize>(view_id.offset),
        view_id.size ? view_id.size : GetSubResourceDataSize(view_id.subresource_index)
    );

    return std::make_shared<ResourceView::ViewDescriptorVariant>(std::move(buffer_view_desc));
}

} // namespace Methane::Graphics::Vulkan
