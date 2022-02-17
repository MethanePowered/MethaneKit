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

FILE: Methane/Graphics/Vulkan/BufferVK.cpp
Vulkan implementation of the buffer interface.

******************************************************************************/

#include "BufferVK.h"
#include "ContextVK.h"

#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/BufferFactory.hpp>
#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>
#include <iterator>

namespace Methane::Graphics
{

static std::vector<vk::Buffer> GetVulkanBuffers(const Refs<Buffer>& buffer_refs)
{
    META_FUNCTION_TASK();
    std::vector<vk::Buffer> vk_buffers;
    std::transform(buffer_refs.begin(), buffer_refs.end(), std::back_inserter(vk_buffers),
                   [](const Ref<Buffer>& buffer_ref)
                   {
                       const auto& vertex_buffer = static_cast<const BufferVK&>(buffer_ref.get());
                       return vertex_buffer.GetNativeResource();
                   }
    );
    return vk_buffers;
}

static vk::BufferUsageFlags GetVulkanBufferUsageFlags(Buffer::Type buffer_type, Buffer::StorageMode storage_mode)
{
    META_FUNCTION_TASK();
    vk::BufferUsageFlags vk_usage_flags;
    switch(buffer_type)
    {
    case Buffer::Type::Data:     vk_usage_flags |= vk::BufferUsageFlagBits::eStorageBuffer; break;
    case Buffer::Type::Constant: vk_usage_flags |= vk::BufferUsageFlagBits::eUniformBuffer; break;
    case Buffer::Type::Index:    vk_usage_flags |= vk::BufferUsageFlagBits::eIndexBuffer; break;
    case Buffer::Type::Vertex:   vk_usage_flags |= vk::BufferUsageFlagBits::eVertexBuffer; break;
        // Buffer::Type::ReadBack - unsupported
    default: META_UNEXPECTED_ARG_DESCR(buffer_type, "Unsupported buffer type");
    }

    if (storage_mode == Buffer::StorageMode::Private)
        vk_usage_flags |= vk::BufferUsageFlagBits::eTransferDst;

    return vk_usage_flags;
}

Ptr<Buffer> Buffer::CreateVertexBuffer(const Context& context, Data::Size size, Data::Size stride, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Graphics::CreateVertexBuffer<BufferVK>(context, size, stride, is_volatile);
}

Ptr<Buffer> Buffer::CreateIndexBuffer(const Context& context, Data::Size size, PixelFormat format, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Graphics::CreateIndexBuffer<BufferVK>(context, size, format, is_volatile);
}

Ptr<Buffer> Buffer::CreateConstantBuffer(const Context& context, Data::Size size, bool addressable, bool is_volatile, const DescriptorByUsage&)
{
    META_FUNCTION_TASK();
    return Graphics::CreateConstantBuffer<BufferVK>(context, size, addressable, is_volatile);
}

Data::Size Buffer::GetAlignedBufferSize(Data::Size size) noexcept
{
    META_FUNCTION_TASK();
    return size;
}

BufferVK::BufferVK(const ContextBase& context, const Settings& settings)
    : ResourceVK(context, settings,
                 dynamic_cast<const IContextVK&>(context).GetDeviceVK().GetNativeDevice().createBufferUnique(
                     vk::BufferCreateInfo(
                         vk::BufferCreateFlags{},
                         settings.size,
                         GetVulkanBufferUsageFlags(settings.type, settings.storage_mode),
                         vk::SharingMode::eExclusive)))
{
    META_FUNCTION_TASK();
    const bool is_private_storage = settings.storage_mode == Buffer::StorageMode::Private;
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

void BufferVK::SetData(const SubResources& sub_resources, CommandQueue* sync_cmd_queue)
{
    META_FUNCTION_TASK();
    ResourceVK::SetData(sub_resources, sync_cmd_queue);

    const bool is_private_storage = GetSettings().storage_mode == Buffer::StorageMode::Private;
    if (is_private_storage)
    {
        m_vk_copy_regions.clear();
        m_vk_copy_regions.reserve(sub_resources.size());
    }

    const vk::DeviceMemory& vk_device_memory = is_private_storage ? m_vk_unique_staging_memory.get() : GetNativeDeviceMemory();
    for(const SubResource& sub_resource : sub_resources)
    {
        ValidateSubResource(sub_resource);

        // TODO: calculate memory offset by sub-resource index
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
    const BlitCommandListVK& upload_cmd_list = PrepareResourceUpload();
    upload_cmd_list.GetNativeCommandBufferDefault().copyBuffer(m_vk_unique_staging_buffer.get(), GetNativeResource(), m_vk_copy_regions);
    FinishResourceUpload(sync_cmd_queue);
    GetContext().RequestDeferredAction(Context::DeferredAction::UploadResources);
}

// ObjectBase overide
bool BufferVK::SetName(const std::string& name)
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

Ptr<BufferSet> BufferSet::Create(Buffer::Type buffers_type, const Refs<Buffer>& buffer_refs)
{
    META_FUNCTION_TASK();
    return std::make_shared<BufferSetVK>(buffers_type, buffer_refs);
}

BufferSetVK::BufferSetVK(Buffer::Type buffers_type, const Refs<Buffer>& buffer_refs)
    : BufferSetBase(buffers_type, buffer_refs)
    , m_vk_buffers(GetVulkanBuffers(buffer_refs))
    , m_vk_offsets(m_vk_buffers.size(), 0U)
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics
