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

FILE: Methane/Graphics/DirectX12/RenderCommandListDX.cpp
DirectX 12 implementation of the render command list interface.

******************************************************************************/

#include "RenderCommandListDX.h"
#include "ParallelRenderCommandListDX.h"
#include "RenderStateDX.h"
#include "RenderPassDX.h"
#include "CommandQueueDX.h"
#include "DeviceDX.h"
#include "ProgramDX.h"
#include "BufferDX.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>
#include <Methane/Graphics/Windows/Helpers.h>

#include <d3dx12.h>
#include <cassert>

namespace Methane::Graphics
{

static D3D12_PRIMITIVE_TOPOLOGY PrimitiveToDXTopology(RenderCommandList::Primitive primitive) noexcept
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

Ptr<RenderCommandList> RenderCommandList::Create(CommandQueue& cmd_queue, RenderPass& render_pass)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<RenderCommandListDX>(static_cast<CommandQueueBase&>(cmd_queue), static_cast<RenderPassBase&>(render_pass));
}

Ptr<RenderCommandList> RenderCommandList::Create(ParallelRenderCommandList& parallel_render_command_list)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<RenderCommandListDX>(static_cast<ParallelRenderCommandListBase&>(parallel_render_command_list));
}

RenderCommandListDX::RenderCommandListDX(CommandQueueBase& cmd_buffer, RenderPassBase& render_pass)
    : CommandListDX<RenderCommandListBase>(cmd_buffer, render_pass)
{
    ITT_FUNCTION_TASK();
}

RenderCommandListDX::RenderCommandListDX(ParallelRenderCommandListBase& parallel_render_command_list)
    : CommandListDX<RenderCommandListBase>(parallel_render_command_list)
{
    ITT_FUNCTION_TASK();
}

void RenderCommandListDX::ResetNative(const Ptr<RenderState>& sp_render_state)
{
    // Reset command list
    if (!IsCommitted())
        return;

    SetCommitted(false);
    ResetCommandState();

    ID3D12PipelineState* p_dx_initial_state = sp_render_state ? static_cast<RenderStateDX&>(*sp_render_state).GetNativePipelineState().Get() : nullptr;
    ID3D12CommandAllocator& dx_cmd_allocator = GetNativeCommandAllocatorRef();
    ThrowIfFailed(dx_cmd_allocator.Reset());
    ThrowIfFailed(GetNativeCommandListRef().Reset(&dx_cmd_allocator, p_dx_initial_state));

    if (!sp_render_state)
        return;

    DrawingState& drawing_state = GetDrawingState();
    drawing_state.p_render_state     = static_cast<RenderStateBase*>(sp_render_state.get());
    drawing_state.render_state_groups = RenderState::Group::Program
                                      | RenderState::Group::Rasterizer
                                      | RenderState::Group::DepthStencil;
}

void RenderCommandListDX::Reset(const Ptr<RenderState>& sp_render_state, const std::string& debug_group)
{
    ITT_FUNCTION_TASK();

    ResetNative(sp_render_state);

    RenderCommandListBase::Reset(sp_render_state, debug_group);

    RenderPassDX& pass_dx = GetPassDX();
    if (IsParallel())
    {
        pass_dx.SetNativeDescriptorHeaps(*this);
        pass_dx.SetNativeRenderTargets(*this);
    }
    else if (!pass_dx.IsBegun())
    {
        pass_dx.Begin(*this);
    }
}

void RenderCommandListDX::SetVertexBuffers(const Refs<Buffer>& vertex_buffers)
{
    ITT_FUNCTION_TASK();

    RenderCommandListBase::SetVertexBuffers(vertex_buffers);

    if (!GetDrawingState().flags.vertex_buffers_changed)
        return;

    std::vector<D3D12_VERTEX_BUFFER_VIEW> vertex_buffer_views;
    vertex_buffer_views.reserve(vertex_buffers.size());
    for (auto vertex_buffer_ref : vertex_buffers)
    {
        assert(vertex_buffer_ref.get().GetBufferType() == Buffer::Type::Vertex);
        const VertexBufferDX& dx_vertex_buffer = static_cast<const VertexBufferDX&>(vertex_buffer_ref.get());
        vertex_buffer_views.push_back(dx_vertex_buffer.GetNativeView());
    }

    GetNativeCommandListRef().IASetVertexBuffers(0, static_cast<UINT>(vertex_buffer_views.size()), vertex_buffer_views.data());
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

    ID3D12GraphicsCommandList& dx_command_list = GetNativeCommandListRef();
    DrawingState& drawing_state = GetDrawingState();
    if (drawing_state.flags.primitive_type_changed)
    {
        const D3D12_PRIMITIVE_TOPOLOGY primitive_topology = PrimitiveToDXTopology(primitive);
        dx_command_list.IASetPrimitiveTopology(primitive_topology);
    }
    if (drawing_state.flags.index_buffer_changed)
    {
        dx_command_list.IASetIndexBuffer(&dx_index_buffer.GetNativeView());
    }
    dx_command_list.DrawIndexedInstanced(index_count, instance_count, start_index, start_vertex, start_instance);
}

void RenderCommandListDX::Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
                               uint32_t instance_count, uint32_t start_instance)
{
    ITT_FUNCTION_TASK();

    RenderCommandListBase::Draw(primitive, vertex_count, start_vertex, instance_count, start_instance);

    ID3D12GraphicsCommandList& dx_command_list = GetNativeCommandListRef();
    if (GetDrawingState().flags.primitive_type_changed)
    {
        const D3D12_PRIMITIVE_TOPOLOGY primitive_topology = PrimitiveToDXTopology(primitive);
        dx_command_list.IASetPrimitiveTopology(primitive_topology);
    }
    dx_command_list.DrawInstanced(vertex_count, instance_count, start_vertex, start_instance);
}

void RenderCommandListDX::Commit()
{
    ITT_FUNCTION_TASK();

    if (!IsParallel())
    {
        RenderPassDX& pass_dx = GetPassDX();
        if (pass_dx.IsBegun())
        {
            pass_dx.End(*this);
        }
    }

    CommandListDX<RenderCommandListBase>::Commit();
}

RenderPassDX& RenderCommandListDX::GetPassDX()
{
    ITT_FUNCTION_TASK();
    return static_cast<RenderPassDX&>(GetPass());
}

} // namespace Methane::Graphics
