/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/DirectX12/BufferDX.h
DirectX 12 implementation of the buffer interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/BufferBase.h>
#include <Methane/Graphics/DescriptorHeap.h>
#include <Methane/Graphics/Windows/Primitives.h>
#include <Methane/Instrumentation.h>

#include <d3dx12.h>
#include <cassert>

namespace Methane::Graphics
{

template<typename TViewNative, typename... ExtraViewArgs>
class BufferDX final : public BufferBase
{
public:
    BufferDX(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage, ExtraViewArgs... view_args)
        : BufferBase(context, settings, descriptor_by_usage)
    {
        META_FUNCTION_TASK();

        const bool             is_read_back_buffer = settings.usage_mask & Usage::CpuReadBack;
        const D3D12_HEAP_TYPE       heap_type      = is_read_back_buffer ? D3D12_HEAP_TYPE_READBACK : D3D12_HEAP_TYPE_UPLOAD;
        const D3D12_RESOURCE_STATES resource_state = is_read_back_buffer ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ;

        InitializeDefaultDescriptors();
        InitializeCommittedResource(CD3DX12_RESOURCE_DESC::Buffer(settings.size), heap_type, resource_state);
        InitializeView(view_args...);
    }

    // Resource interface
    void SetData(const SubResources& sub_resources) override
    {
        META_FUNCTION_TASK();
        BufferBase::SetData(sub_resources);

        ID3D12Resource& d3d12_resource = GetNativeResourceRef();
        for(const SubResource& sub_resource : sub_resources)
        {
            CD3DX12_RANGE read_range; // Zero range, since we're not going to read this resource on CPU
            Data::RawPtr p_sub_resource_data = nullptr;

            ThrowIfFailed(d3d12_resource.Map(sub_resource.index.GetRawIndex(),
                          &read_range, reinterpret_cast<void**>(&p_sub_resource_data)),
                          GetContextDX().GetDeviceDX().GetNativeDevice().Get());

            if (!p_sub_resource_data)
                throw std::runtime_error("Failed to map buffer subresource.");

            stdext::checked_array_iterator<Data::RawPtr> sub_resource_data_it(p_sub_resource_data, sub_resource.size);
            std::copy(sub_resource.p_data, sub_resource.p_data + sub_resource.size, sub_resource_data_it);

            d3d12_resource.Unmap(sub_resource.index.GetRawIndex(), nullptr);
        }
    }

    Data::Chunk GetData(const SubResource::Index& sub_resource_index = SubResource::Index(), const BytesRange& data_range = BytesRange()) override
    {
        META_FUNCTION_TASK();
        ValidateSubResourceIndex(sub_resource_index);

        ID3D12Resource& d3d12_resource = GetNativeResourceRef();
        const CD3DX12_RANGE read_range(data_range.GetStart(), data_range.GetEnd());
        Data::RawPtr p_sub_resource_data = nullptr;

        ThrowIfFailed(d3d12_resource.Map(sub_resource_index.GetRawIndex(),
                      &read_range, reinterpret_cast<void**>(&p_sub_resource_data)),
                      GetContextDX().GetDeviceDX().GetNativeDevice().Get());

        if (!p_sub_resource_data)
            throw std::runtime_error("Failed to map buffer subresource.");

        stdext::checked_array_iterator<Data::RawPtr> sub_resource_data_it(p_sub_resource_data, data_range.GetLength());
        Data::Bytes sub_resource_data(data_range.GetLength(), 0);
        std::copy(sub_resource_data_it, sub_resource_data_it + data_range.GetLength(), sub_resource_data.begin());

        d3d12_resource.Unmap(sub_resource_index.GetRawIndex(), nullptr);

        return Data::Chunk(std::move(sub_resource_data));
    }

    const TViewNative& GetNativeView() const { return m_buffer_view; }

protected:
    void InitializeView(ExtraViewArgs...);

private:
    // NOTE: in case of resource context placed in descriptor heap, m_buffer_view field holds context descriptor instead of context
    TViewNative m_buffer_view;
};

struct ReadBackBufferView { };

using VertexBufferDX = BufferDX<D3D12_VERTEX_BUFFER_VIEW, Data::Size>;
using IndexBufferDX = BufferDX<D3D12_INDEX_BUFFER_VIEW, PixelFormat>;
using ConstantBufferDX = BufferDX<D3D12_CONSTANT_BUFFER_VIEW_DESC>;
using ReadBackBufferDX = BufferDX<ReadBackBufferView>;

class BuffersDX final : public BuffersBase
{
public:
    BuffersDX(Buffer::Type buffers_type, Refs<Buffer> buffer_refs);

    const std::vector<D3D12_VERTEX_BUFFER_VIEW>& GetNativeVertexBufferViews() const;

private:
    std::vector<D3D12_VERTEX_BUFFER_VIEW> m_vertex_buffer_views;
};

} // namespace Methane::Graphics
