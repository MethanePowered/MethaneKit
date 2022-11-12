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
#include <Methane/Graphics/Vulkan/IContextVk.h>

#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/Base/BufferFactory.hpp>
#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>
#include <iterator>

namespace Methane::Graphics
{

Ptr<IBuffer> IBuffer::CreateVertexBuffer(const IContext& context, Data::Size size, Data::Size stride, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Base::CreateVertexBuffer<Vulkan::Buffer>(context, size, stride, is_volatile);
}

Ptr<IBuffer> IBuffer::CreateIndexBuffer(const IContext& context, Data::Size size, PixelFormat format, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Base::CreateIndexBuffer<Vulkan::Buffer>(context, size, format, is_volatile);
}

Ptr<IBuffer> IBuffer::CreateConstantBuffer(const IContext& context, Data::Size size, bool addressable, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Base::CreateConstantBuffer<Vulkan::Buffer>(context, size, addressable, is_volatile);
}

Data::Size IBuffer::GetAlignedBufferSize(Data::Size size) noexcept
{
    META_FUNCTION_TASK();
    return size;
}

Ptr<IBufferSet> IBufferSet::Create(IBuffer::Type buffers_type, const Refs<IBuffer>& buffer_refs)
{
    META_FUNCTION_TASK();
    return std::make_shared<Vulkan::BufferSet>(buffers_type, buffer_refs);
}

} // namespace Methane::Graphics

namespace Methane::Graphics::Vulkan
{

static std::vector<vk::Buffer> GetVulkanBuffers(const Refs<IBuffer>& buffer_refs)
{
    META_FUNCTION_TASK();
    std::vector<vk::Buffer> vk_buffers;
    std::transform(buffer_refs.begin(), buffer_refs.end(), std::back_inserter(vk_buffers),
                   [](const Ref<IBuffer>& buffer_ref)
                   {
                       const auto& vertex_buffer = static_cast<const Buffer&>(buffer_ref.get());
                       return vertex_buffer.GetNativeResource();
                   }
    );
    return vk_buffers;
}

static vk::BufferUsageFlags GetVulkanBufferUsageFlags(IBuffer::Type buffer_type, IBuffer::StorageMode storage_mode)
{
    META_FUNCTION_TASK();
    vk::BufferUsageFlags vk_usage_flags;
    switch(buffer_type)
    {
    case IBuffer::Type::Storage: vk_usage_flags |= vk::BufferUsageFlagBits::eStorageBuffer; break;
    case IBuffer::Type::Constant: vk_usage_flags |= vk::BufferUsageFlagBits::eUniformBuffer; break;
    case IBuffer::Type::Index: vk_usage_flags |= vk::BufferUsageFlagBits::eIndexBuffer; break;
    case IBuffer::Type::Vertex: vk_usage_flags |= vk::BufferUsageFlagBits::eVertexBuffer; break;
        // Buffer::Type::ReadBack - unsupported
    default: META_UNEXPECTED_ARG_DESCR(buffer_type, "Unsupported buffer type");
    }

    if (storage_mode == IBuffer::StorageMode::Private)
        vk_usage_flags |= vk::BufferUsageFlagBits::eTransferDst;

    return vk_usage_flags;
}

static IResource::State GetTargetResourceStateByBufferType(IBuffer::Type buffer_type)
{
    META_FUNCTION_TASK();
    switch(buffer_type)
    {
    case IBuffer::Type::Storage:     return IResource::State::ShaderResource;
    case IBuffer::Type::Constant:    return IResource::State::ConstantBuffer;
    case IBuffer::Type::Index:       return IResource::State::IndexBuffer;
    case IBuffer::Type::Vertex:      return IResource::State::VertexBuffer;
    case IBuffer::Type::ReadBack:    return IResource::State::StreamOut;
    default: META_UNEXPECTED_ARG_DESCR_RETURN(buffer_type, IResource::State::Undefined, "Unsupported buffer type");
    }
}

Buffer::Buffer(const Base::Context& context, const Settings& settings)
    : Resource(context, settings,
                 dynamic_cast<const IContextVk&>(context).GetVulkanDevice().GetNativeDevice().createBufferUnique(
                     vk::BufferCreateInfo(
                         vk::BufferCreateFlags{},
                         settings.size,
                         GetVulkanBufferUsageFlags(settings.type, settings.storage_mode),
                         vk::SharingMode::eExclusive)))
{
    META_FUNCTION_TASK();
    const bool is_private_storage = settings.storage_mode == IBuffer::StorageMode::Private;
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

void Buffer::SetData(const SubResources& sub_resources, ICommandQueue& target_cmd_queue)
{
    META_FUNCTION_TASK();
    Resource::SetData(sub_resources, target_cmd_queue);

    const Settings& buffer_settings = GetSettings();
    const bool is_private_storage = buffer_settings.storage_mode == IBuffer::StorageMode::Private;
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
    TransferCommandList& upload_cmd_list = PrepareResourceUpload(target_cmd_queue);
    upload_cmd_list.GetNativeCommandBufferDefault().copyBuffer(m_vk_unique_staging_buffer.get(), GetNativeResource(), m_vk_copy_regions);
    CompleteResourceUpload(upload_cmd_list, GetTargetResourceStateByBufferType(buffer_settings.type), target_cmd_queue);
    GetContext().RequestDeferredAction(IContext::DeferredAction::UploadResources);
}

bool Buffer::SetName(const std::string& name)
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

BufferSet::BufferSet(IBuffer::Type buffers_type, const Refs<IBuffer>& buffer_refs)
    : Base::BufferSet(buffers_type, buffer_refs)
    , m_vk_buffers(GetVulkanBuffers(buffer_refs))
    , m_vk_offsets(m_vk_buffers.size(), 0U)
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics::Vulkan
