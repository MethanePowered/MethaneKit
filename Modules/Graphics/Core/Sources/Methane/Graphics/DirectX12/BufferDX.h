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
#include <Methane/Graphics/Windows/Helpers.h>
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
        ITT_FUNCTION_TASK();
        InitializeDefaultDescriptors();
        InitializeCommittedResource(CD3DX12_RESOURCE_DESC::Buffer(settings.size), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
        InitializeView(view_args...);
    }

    // Resource interface
    void SetData(const SubResources& sub_resources) override
    {
        ITT_FUNCTION_TASK();
        BufferBase::SetData(sub_resources);

        for(const SubResource& sub_resource : sub_resources)
        {
            ID3D12Resource& d3d12_resource = GetNativeResourceRef();
            char* p_resource_data = nullptr;
            CD3DX12_RANGE read_range(0, 0); // Zero range, since we're not going to read this resource on CPU
            ThrowIfFailed(d3d12_resource.Map(sub_resource.GetRawIndex(), &read_range, reinterpret_cast<void**>(&p_resource_data)));

            assert(!!p_resource_data);
            std::copy(sub_resource.p_data, sub_resource.p_data + sub_resource.data_size, stdext::checked_array_iterator<char*>(p_resource_data, GetDataSize()));

            d3d12_resource.Unmap(sub_resource.GetRawIndex(), nullptr);
        }
    }

    // Buffer interface
    uint32_t GetFormattedItemsCount() const override { return m_formatted_items_count; }

    const TViewNative& GetNativeView() const { return m_buffer_view; }

protected:
    void InitializeView(ExtraViewArgs...);

private:
    // NOTE: in case of resource context placed in descriptor heap, m_buffer_view field holds context descriptor instead of context
    TViewNative m_buffer_view;
    uint32_t    m_formatted_items_count = 0;
};

using VertexBufferDX = BufferDX<D3D12_VERTEX_BUFFER_VIEW, Data::Size>;
using IndexBufferDX = BufferDX<D3D12_INDEX_BUFFER_VIEW, PixelFormat>;
using ConstantBufferDX = BufferDX<D3D12_CONSTANT_BUFFER_VIEW_DESC>;

} // namespace Methane::Graphics
