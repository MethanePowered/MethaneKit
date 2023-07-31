/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/DirectX/Buffer.cpp
DirectX 12 implementation of the buffer interface.

******************************************************************************/

#include "Methane/Graphics/RHI/ResourceView.h"
#include <Methane/Graphics/DirectX/Buffer.h>
#include <Methane/Graphics/DirectX/Device.h>
#include <Methane/Graphics/DirectX/Types.h>
#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/TypeConverters.hpp>

#include <Methane/Data/Math.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>
#include <directx/d3dx12_core.h>

namespace Methane::Graphics::DirectX
{

static Rhi::BufferSettings UpdateBufferSettings(const Rhi::BufferSettings& settings)
{
    Rhi::BufferSettings new_settings = settings;
    if (new_settings.type == Rhi::BufferType::Constant ||
        new_settings.type == Rhi::BufferType::Storage)
    {
        new_settings.size = Data::AlignUp(settings.size, Data::Size(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
    }
    return new_settings;
}

Buffer::Buffer(const Base::Context& context, const Settings& orig_settings)
    : Resource(context, UpdateBufferSettings(orig_settings))
{
    META_FUNCTION_TASK();
    const Settings&     settings            = GetSettings();
    const bool          is_private_storage  = settings.storage_mode == IBuffer::StorageMode::Private;
    const bool          is_read_back_buffer = settings.usage_mask.HasAnyBit(Usage::ReadBack);
    const D3D12_HEAP_TYPE  normal_heap_type = is_private_storage ? D3D12_HEAP_TYPE_DEFAULT  : D3D12_HEAP_TYPE_UPLOAD;
    const D3D12_HEAP_TYPE  heap_type        = is_read_back_buffer ? D3D12_HEAP_TYPE_READBACK : normal_heap_type;
    const Rhi::ResourceState resource_state = is_read_back_buffer || is_private_storage
                                              ? Rhi::ResourceState::Common
                                              : Rhi::ResourceState::GenericRead;

    D3D12_RESOURCE_FLAGS resource_flags = D3D12_RESOURCE_FLAG_NONE;
    if (settings.usage_mask.HasAnyBit(Usage::ShaderWrite))
        resource_flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    CD3DX12_RESOURCE_DESC resource_desc = CD3DX12_RESOURCE_DESC::Buffer(settings.size, resource_flags);
    InitializeCommittedResource(resource_desc, heap_type, resource_state);

    if (is_private_storage)
    {
        resource_desc.Width = Data::AlignUp(resource_desc.Width, UINT64(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
        m_cp_upload_resource = CreateCommittedResource(resource_desc, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
    }

    // Resources on D3D12_HEAP_TYPE_UPLOAD heaps requires D3D12_RESOURCE_STATE_GENERIC_READ or D3D12_RESOURCE_STATE_RESOLVE_SOURCE, which can not be changed.
    // SetState(state, barriers) behavior prevents updating resource barriers while setting a given state
    SetStateChangeUpdatesBarriers(is_private_storage);
}

bool Buffer::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Resource::SetName(name))
        return false;

    if (m_cp_upload_resource)
    {
        m_cp_upload_resource->SetName(nowide::widen(fmt::format("{} Upload Resource", name)).c_str());
    }
    return true;
}

void Buffer::SetData(Rhi::ICommandQueue& target_cmd_queue, const SubResource& sub_resource)
{
    META_FUNCTION_TASK();
    Resource::SetData(target_cmd_queue, sub_resource);

    const Settings&     settings = GetSettings();
    const CD3DX12_RANGE zero_read_range(0U, 0U);
    const bool       is_private_storage = settings.storage_mode == IBuffer::StorageMode::Private;
    ID3D12Resource&      d3d12_resource = is_private_storage ? *m_cp_upload_resource.Get() : GetNativeResourceRef();

    // Using zero range, since we're not going to read this resource on CPU
    const Data::Index sub_resource_raw_index = sub_resource.GetIndex().GetRawIndex(Rhi::SubResourceCount());
    Data::RawPtr      p_sub_resource_data    = nullptr;
    ThrowIfFailed(
        d3d12_resource.Map(sub_resource_raw_index, &zero_read_range,
                           reinterpret_cast<void**>(&p_sub_resource_data)), // NOSONAR
        GetDirectContext().GetDirectDevice().GetNativeDevice().Get()
    );

    META_CHECK_ARG_NOT_NULL_DESCR(p_sub_resource_data, "failed to map buffer subresource");
    stdext::checked_array_iterator target_data_it(p_sub_resource_data, sub_resource.GetDataSize());
    std::copy(sub_resource.GetDataPtr(), sub_resource.GetDataEndPtr(), target_data_it);

    if (sub_resource.HasDataRange())
    {
        const CD3DX12_RANGE write_range(sub_resource.GetDataRange().GetStart(), sub_resource.GetDataRange().GetEnd());
        d3d12_resource.Unmap(sub_resource_raw_index, &write_range);
    }
    else
    {
        d3d12_resource.Unmap(sub_resource_raw_index, nullptr);
    }

    if (!is_private_storage)
        return;

    // In case of private GPU storage, copy buffer data from intermediate upload resource to the private GPU resource
    const TransferCommandList& upload_cmd_list = PrepareResourceTransfer(TransferOperation::Upload, target_cmd_queue, State::CopyDest);
    upload_cmd_list.GetNativeCommandList().CopyBufferRegion(GetNativeResource(), 0U, m_cp_upload_resource.Get(), 0U, settings.size);
    GetContext().RequestDeferredAction(Rhi::IContext::DeferredAction::UploadResources);
}

Rhi::SubResource Buffer::GetData(Rhi::ICommandQueue&, const BytesRangeOpt& data_range)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_TRUE_DESCR(GetUsage().HasAnyBit(Rhi::ResourceUsage::ReadBack),
                              "getting buffer data from GPU is allowed for buffers with CPU Read-back flag only");

    const Data::Index data_start  = data_range ? data_range->GetStart()  : 0U;
    const Data::Index data_length = data_range ? data_range->GetLength() : GetDataSize();
    const Data::Index data_end    = data_start + data_length;

    ID3D12Resource& d3d12_resource = GetNativeResourceRef();
    const CD3DX12_RANGE read_range(data_start, data_start + data_length);
    Data::RawPtr sub_resource_data_ptr = nullptr;
    ThrowIfFailed(
        d3d12_resource.Map(0U, &read_range, reinterpret_cast<void**>(&sub_resource_data_ptr)), // NOSONAR
        GetDirectContext().GetDirectDevice().GetNativeDevice().Get()
    );

    META_CHECK_ARG_NOT_NULL_DESCR(sub_resource_data_ptr, "failed to map buffer subresource");

    stdext::checked_array_iterator source_data_it(sub_resource_data_ptr, data_end);
    Data::Bytes                    sub_resource_data(data_length, {});
    std::copy(source_data_it + data_start, source_data_it + data_end, sub_resource_data.begin());

    const CD3DX12_RANGE zero_write_range(0, 0);
    d3d12_resource.Unmap(0U, &zero_write_range);

    return SubResource(std::move(sub_resource_data), Rhi::SubResourceIndex(), data_range);
}

D3D12_VERTEX_BUFFER_VIEW Buffer::GetNativeVertexBufferView() const
{
    META_FUNCTION_TASK();
    const Rhi::BufferSettings& settings = GetSettings();
    META_CHECK_ARG_EQUAL(settings.type, Rhi::BufferType::Vertex);

    D3D12_VERTEX_BUFFER_VIEW buffer_view{};
    buffer_view.BufferLocation = GetNativeGpuAddress();
    buffer_view.SizeInBytes    = GetDataSize();
    buffer_view.StrideInBytes  = settings.item_stride_size;
    return buffer_view;
}

D3D12_INDEX_BUFFER_VIEW Buffer::GetNativeIndexBufferView() const
{
    META_FUNCTION_TASK();
    const Rhi::BufferSettings& settings = GetSettings();
    META_CHECK_ARG_EQUAL(settings.type, Rhi::BufferType::Index);

    D3D12_INDEX_BUFFER_VIEW buffer_view{};
    buffer_view.BufferLocation = GetNativeGpuAddress();
    buffer_view.SizeInBytes    = GetDataSize();
    buffer_view.Format         = TypeConverter::PixelFormatToDxgi(settings.data_format);
    return buffer_view;
}

D3D12_CONSTANT_BUFFER_VIEW_DESC Buffer::GetNativeConstantBufferViewDesc() const
{
    META_FUNCTION_TASK();
    const Rhi::BufferSettings& settings = GetSettings();
    META_CHECK_ARG_EQUAL(settings.type, Rhi::BufferType::Constant);

    D3D12_CONSTANT_BUFFER_VIEW_DESC buffer_view_desc{};
    buffer_view_desc.BufferLocation = GetNativeGpuAddress();
    buffer_view_desc.SizeInBytes    = GetDataSize();
    return buffer_view_desc;
}

Opt<Rhi::IResource::Descriptor> Buffer::InitializeNativeViewDescriptor(const View::Id& view_id)
{
    META_FUNCTION_TASK();
    if (GetSettings().type != Rhi::BufferType::Constant)
        return std::nullopt;

    // NOTE: Addressable resources are bound to pipeline using GPU Address and byte offset
    if (const UsageMask usage_mask = GetUsage();
        !usage_mask.HasAnyBit(Usage::ShaderRead) || usage_mask.HasAnyBit(Usage::Addressable))
        return std::nullopt;

    const Rhi::IResource::Descriptor& descriptor = GetDescriptorByViewId(view_id);
    const D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle = GetNativeCpuDescriptorHandle(descriptor);
    const D3D12_CONSTANT_BUFFER_VIEW_DESC view_desc = GetNativeConstantBufferViewDesc();
    GetDirectContext().GetDirectDevice().GetNativeDevice()->CreateConstantBufferView(&view_desc, cpu_descriptor_handle);
    return descriptor;
}

} // namespace Methane::Graphics
