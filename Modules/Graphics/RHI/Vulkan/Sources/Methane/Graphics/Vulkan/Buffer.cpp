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

void Buffer::SetData(Rhi::ICommandQueue& target_cmd_queue, const Rhi::SubResource& sub_resource)
{
    META_FUNCTION_TASK();
    Base::Buffer::SetData(target_cmd_queue, sub_resource);

    const Settings& buffer_settings = GetSettings();
    const bool is_private_storage = buffer_settings.storage_mode == Rhi::IBuffer::StorageMode::Private;
    const vk::DeviceMemory& vk_device_memory = is_private_storage ? m_vk_unique_staging_memory.get() : GetNativeDeviceMemory();

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
        m_vk_copy_region = vk::BufferCopy(sub_resource_offset, sub_resource_offset, static_cast<vk::DeviceSize>(sub_resource.GetDataSize()));
    }

    if (!is_private_storage)
        return;

    // In case of private GPU storage, copy buffer data from staging upload resource to the device-local GPU resource
    TransferCommandList& upload_cmd_list = PrepareResourceTransfer(target_cmd_queue, State::CopyDest);
    upload_cmd_list.GetNativeCommandBufferDefault().copyBuffer(m_vk_unique_staging_buffer.get(), GetNativeResource(), 1U, &m_vk_copy_region);
    CompleteResourceTransfer(upload_cmd_list, GetTargetResourceStateByBufferType(buffer_settings.type), target_cmd_queue);
    GetContext().RequestDeferredAction(Rhi::ContextDeferredAction::UploadResources);
}

Rhi::SubResource Buffer::GetData(Rhi::ICommandQueue& target_cmd_queue, const BytesRangeOpt& data_range)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_TRUE_DESCR(GetUsage().HasAnyBit(Rhi::ResourceUsage::ReadBack),
                              "getting buffer data from GPU is allowed for buffers with CPU Read-back flag only");

    const BytesRange buffer_data_range(data_range ? data_range->GetStart() : 0U,
                                       data_range ? data_range->GetEnd()   : GetDataSize());

    Data::Bytes data;
    switch(GetSettings().storage_mode)
    {
    case IBuffer::StorageMode::Managed: data = GetDataFromSharedBuffer(buffer_data_range); break;
    case IBuffer::StorageMode::Private: data = GetDataFromPrivateBuffer(buffer_data_range, target_cmd_queue); break;
    default: META_UNEXPECTED_ARG_RETURN(GetSettings().storage_mode, SubResource());
    }

    return Rhi::SubResource(std::move(data), Rhi::SubResourceIndex(), data_range);
}

Data::Bytes Buffer::GetDataFromSharedBuffer(const BytesRange& data_range) const
{
    META_FUNCTION_TASK();
    Data::RawPtr data_ptr = nullptr;
    const vk::DeviceMemory& vk_device_memory = GetNativeDeviceMemory();
    const vk::Result vk_map_result = GetNativeDevice().mapMemory(vk_device_memory, data_range.GetStart(), data_range.GetLength(),
                                                                 vk::MemoryMapFlags{}, reinterpret_cast<void**>(&data_ptr)); // NOSONAR

    META_CHECK_ARG_EQUAL_DESCR(vk_map_result, vk::Result::eSuccess, "failed to map buffer subresource");
    META_CHECK_ARG_NOT_NULL_DESCR(data_ptr, "failed to map buffer subresource");
    Data::Bytes data(data_ptr, data_ptr + data_range.GetLength());
    GetNativeDevice().unmapMemory(vk_device_memory);

    return data;
}

Data::Bytes Buffer::GetDataFromPrivateBuffer(const BytesRange& data_range, Rhi::ICommandQueue& target_cmd_queue)
{
    META_FUNCTION_TASK();
    const State       initial_buffer_state = GetState();
    TransferCommandList&   upload_cmd_list = PrepareResourceTransfer(target_cmd_queue, State::CopySource);
    const vk::CommandBuffer& vk_cmd_buffer = upload_cmd_list.GetNativeCommandBufferDefault();
    const vk::BufferCopy vk_buffer_copy(data_range.GetStart(), 0U, data_range.GetLength());
    vk_cmd_buffer.copyBuffer(GetNativeResource(), m_vk_unique_staging_buffer.get(), 1U, &vk_buffer_copy);

    CompleteResourceTransfer(upload_cmd_list, initial_buffer_state, target_cmd_queue);

    // Execute resource transfer commands and wait for completion
    GetBaseContext().UploadResources();

    // Copy buffer data from mapped staging resource
    Data::RawPtr data_ptr = nullptr;
    const vk::Result vk_map_result = GetNativeDevice().mapMemory(m_vk_unique_staging_memory.get(), 0U, data_range.GetLength(),
                                                                 vk::MemoryMapFlags{}, reinterpret_cast<void**>(&data_ptr)); // NOSONAR
    META_CHECK_ARG_EQUAL_DESCR(vk_map_result, vk::Result::eSuccess, "failed to map buffer subresource");
    META_CHECK_ARG_NOT_NULL_DESCR(data_ptr, "failed to map buffer subresource");
    Data::Bytes data(data_ptr, data_ptr + data_range.GetLength());
    GetNativeDevice().unmapMemory(m_vk_unique_staging_memory.get());

    return data;
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
        view_id.size ? view_id.size : GetDataSize()
    );

    return std::make_shared<ResourceView::ViewDescriptorVariant>(std::move(buffer_view_desc));
}

} // namespace Methane::Graphics::Vulkan
