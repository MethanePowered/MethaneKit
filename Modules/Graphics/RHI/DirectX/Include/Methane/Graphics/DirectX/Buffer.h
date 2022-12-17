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

FILE: Methane/Graphics/DirectX/Buffer.h
DirectX 12 implementation of the buffer interface.

******************************************************************************/

#pragma once

#include "TransferCommandList.h"
#include "Resource.hpp"
#include "DescriptorHeap.h"
#include "ErrorHandling.h"

#include <Methane/Graphics/Base/Buffer.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <fmt/format.h>
#include <directx/d3dx12.h>

namespace Methane::Graphics::DirectX
{

class Buffer final // NOSONAR - inheritance hierarchy is greater than 5
    : public Resource<Base::Buffer>
{
public:
    Buffer(const Base::Context& context, const Settings& settings);

    // IObject overrides
    bool SetName(std::string_view name) override;

    // IResource overrides
    void SetData(const SubResources& sub_resources, Rhi::ICommandQueue& target_cmd_queue) override;
    SubResource GetData(const SubResource::Index& sub_resource_index = SubResource::Index(), const std::optional<BytesRange>& data_range = {}) override;
    Opt<Descriptor> InitializeNativeViewDescriptor(const View::Id& view_id) override;

    D3D12_VERTEX_BUFFER_VIEW        GetNativeVertexBufferView() const;
    D3D12_INDEX_BUFFER_VIEW         GetNativeIndexBufferView() const;
    D3D12_CONSTANT_BUFFER_VIEW_DESC GetNativeConstantBufferViewDesc() const;

private:
    wrl::ComPtr<ID3D12Resource> m_cp_upload_resource;
};

class BufferSet final
    : public Base::BufferSet
{
public:
    BufferSet(Rhi::IBuffer::Type buffers_type, const Refs<Rhi::IBuffer>& buffer_refs);

    const std::vector<D3D12_VERTEX_BUFFER_VIEW>& GetNativeVertexBufferViews() const;

private:
    std::vector<D3D12_VERTEX_BUFFER_VIEW> m_vertex_buffer_views;
};

} // namespace Methane::Graphics::DirectX
