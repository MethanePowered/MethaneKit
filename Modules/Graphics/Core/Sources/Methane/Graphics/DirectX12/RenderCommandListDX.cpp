/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/DirectX12/RenderCommandListDX.cpp
DirectX 12 implementation of the render command list interface.

******************************************************************************/

#include "RenderCommandListDX.h"
#include "RenderStateDX.h"
#include "RenderPassDX.h"
#include "CommandQueueDX.h"
#include "ContextDX.h"
#include "DeviceDX.h"
#include "ProgramDX.h"
#include "ResourceDX.h"
#include "TextureDX.h"
#include "BufferDX.h"
#include "TypesDX.h"

#include <Methane/Data/Instrumentation.h>
#include <Methane/Graphics/Windows/Helpers.h>

#include <d3dx12.h>
#include <pix.h>
#include <nowide/convert.hpp>
#include <cassert>

namespace Methane::Graphics
{

D3D12_PRIMITIVE_TOPOLOGY PrimitiveToDXTopology(RenderCommandList::Primitive primitive) noexcept
{
    ITT_FUNCTION_TASK();
    switch (primitive)
    {
    case RenderCommandList::Primitive::Point:          return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case RenderCommandList::Primitive::Line:           return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case RenderCommandList::Primitive::LineStrip:      return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case RenderCommandList::Primitive::Triangle:       return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case RenderCommandList::Primitive::TriangleStrip:  return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    default:                                           assert(0);
    }
    return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

RenderCommandList::Ptr RenderCommandList::Create(CommandQueue& cmd_queue, RenderPass& render_pass)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<RenderCommandListDX>(static_cast<CommandQueueBase&>(cmd_queue), static_cast<RenderPassBase&>(render_pass));
}

RenderCommandListDX::RenderCommandListDX(CommandQueueBase& cmd_buffer, RenderPassBase& render_pass)
    : RenderCommandListBase(cmd_buffer, render_pass)
{
    ITT_FUNCTION_TASK();

    const wrl::ComPtr<ID3D12Device>& cp_device = GetCommandQueueDX().GetContextDX().GetDeviceDX().GetNativeDevice();
    assert(!!cp_device);

    ThrowIfFailed(cp_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cp_command_allocator)));
    ThrowIfFailed(cp_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cp_command_allocator.Get(), nullptr, IID_PPV_ARGS(&m_cp_command_list)));
}

void RenderCommandListDX::Reset(RenderState& render_state, const std::string& debug_group)
{
    ITT_FUNCTION_TASK();

    assert(m_cp_command_list);
    RenderStateDX& dx_state = static_cast<RenderStateDX&>(render_state);

    // Reset command list
    if (m_is_committed)
    {
        m_is_committed = false;
        ThrowIfFailed(m_cp_command_allocator->Reset());
        ThrowIfFailed(m_cp_command_list->Reset(m_cp_command_allocator.Get(), dx_state.GetNativePipelineState().Get()));
        m_is_pass_applied = false;
    }

    RenderCommandListBase::Reset(render_state, debug_group);

    // Set render target transition barriers and apply pass
    if (!m_is_pass_applied)
    {
        RenderPassDX& pass_dx = GetPassDX();
        m_present_resources = pass_dx.GetColorAttachmentResources();

        if (!m_present_resources.empty())
        {
            SetResourceTransitionBarriers(m_present_resources, ResourceBase::State::Present, ResourceBase::State::RenderTarget);
        }

        pass_dx.Apply(*this);
        m_is_pass_applied = true;
    }
}

void RenderCommandListDX::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    RenderCommandListBase::SetName(name);

    assert(m_cp_command_list);
    m_cp_command_list->SetName(nowide::widen(name).c_str());

    assert(m_cp_command_allocator);
    m_cp_command_allocator->SetName(nowide::widen(name + " allocator").c_str());
}

void RenderCommandListDX::PushDebugGroup(const std::string& name)
{
    ITT_FUNCTION_TASK();
    PIXBeginEvent(m_cp_command_list.Get(), 0, nowide::widen(name).c_str());
}

void RenderCommandListDX::PopDebugGroup()
{
    ITT_FUNCTION_TASK();
    PIXEndEvent(m_cp_command_list.Get());
}

void RenderCommandListDX::SetVertexBuffers(const Buffer::Refs& vertex_buffers)
{
    ITT_FUNCTION_TASK();

    RenderCommandListBase::SetVertexBuffers(vertex_buffers);

    if (!m_draw_state.flags.vertex_buffers_changed)
        return;

    std::vector<D3D12_VERTEX_BUFFER_VIEW> vertex_buffer_views;
    vertex_buffer_views.reserve(vertex_buffers.size());
    for (auto vertex_buffer_ref : vertex_buffers)
    {
        assert(vertex_buffer_ref.get().GetBufferType() == Buffer::Type::Vertex);
        const VertexBufferDX& dx_vertex_buffer = static_cast<const VertexBufferDX&>(vertex_buffer_ref.get());
        vertex_buffer_views.push_back(dx_vertex_buffer.GetNativeView());
    }

    assert(m_cp_command_list);
    m_cp_command_list->IASetVertexBuffers(0, static_cast<UINT>(vertex_buffer_views.size()), vertex_buffer_views.data());
}

void RenderCommandListDX::DrawIndexed(Primitive primitive, Buffer& index_buffer,
                                      uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                                      uint32_t instance_count, uint32_t start_instance)
{
    ITT_FUNCTION_TASK();

    const IndexBufferDX& dx_index_buffer = static_cast<IndexBufferDX&>(index_buffer);
    if (!index_count)
    {
        index_count = dx_index_buffer.GetFormattedItemsCount();
    }

    RenderCommandListBase::DrawIndexed(primitive, index_buffer, index_count, start_index, start_vertex, instance_count, start_instance);

    assert(m_cp_command_list);

    if (m_draw_state.flags.primitive_type_changed)
    {
        const D3D12_PRIMITIVE_TOPOLOGY primitive_topology = PrimitiveToDXTopology(primitive);
        m_cp_command_list->IASetPrimitiveTopology(primitive_topology);
    }
    if (m_draw_state.flags.index_buffer_changed)
    {
        m_cp_command_list->IASetIndexBuffer(&dx_index_buffer.GetNativeView());
    }
    m_cp_command_list->DrawIndexedInstanced(index_count, instance_count, start_index, start_vertex, start_instance);
}

void RenderCommandListDX::Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
                               uint32_t instance_count, uint32_t start_instance)
{
    ITT_FUNCTION_TASK();

    RenderCommandListBase::Draw(primitive, vertex_count, start_vertex, instance_count, start_instance);

    assert(m_cp_command_list);

    if (m_draw_state.flags.primitive_type_changed)
    {
        const D3D12_PRIMITIVE_TOPOLOGY primitive_topology = PrimitiveToDXTopology(primitive);
        m_cp_command_list->IASetPrimitiveTopology(primitive_topology);
    }
    m_cp_command_list->DrawInstanced(vertex_count, instance_count, start_vertex, start_instance);
}

void RenderCommandListDX::SetResourceBarriers(const ResourceBase::Barriers& resource_barriers)
{
    ITT_FUNCTION_TASK();

    if (resource_barriers.empty())
    {
        throw std::invalid_argument("Can not set empty resource barriers");
    }

    std::vector<D3D12_RESOURCE_BARRIER> dx_resource_barriers;
    for (const ResourceBase::Barrier& resource_barrier : resource_barriers)
    {
        dx_resource_barriers.push_back(ResourceDX::GetNativeResourceBarrier(resource_barrier));
    }

    assert(m_cp_command_list);
    m_cp_command_list->ResourceBarrier(static_cast<UINT>(dx_resource_barriers.size()), dx_resource_barriers.data());
}

void RenderCommandListDX::Commit(bool present_drawable)
{
    ITT_FUNCTION_TASK();

    if (!m_present_resources.empty())
    {
        SetResourceTransitionBarriers(m_present_resources, ResourceBase::State::RenderTarget, ResourceBase::State::Present);
    }
    m_present_resources.clear();

    RenderCommandListBase::Commit(present_drawable);

    m_cp_command_list->Close();
    m_is_committed = true;
}

CommandQueueDX& RenderCommandListDX::GetCommandQueueDX()
{
    ITT_FUNCTION_TASK();
    return static_cast<CommandQueueDX&>(GetCommandQueueBase());
}

RenderPassDX& RenderCommandListDX::GetPassDX()
{
    ITT_FUNCTION_TASK();
    return static_cast<RenderPassDX&>(GetPass());
}

void RenderCommandListDX::Execute(uint32_t frame_index)
{
    ITT_FUNCTION_TASK();
    
    RenderCommandListBase::Execute(frame_index);

    // NOTE: In DirectX there's no need for tracking command list completion, so it's completed right away
    Complete(frame_index);
}

} // namespace Methane::Graphics
