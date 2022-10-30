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
#include <Methane/Graphics/Windows/DirectXErrorHandling.h>

#include <magic_enum.hpp>
#include <directx/d3dx12.h>

namespace Methane::Graphics
{

static D3D12_PRIMITIVE_TOPOLOGY PrimitiveToDXTopology(RenderCommandList::Primitive primitive)
{
    META_FUNCTION_TASK();
    switch (primitive)
    {
    case RenderCommandList::Primitive::Point:          return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case RenderCommandList::Primitive::Line:           return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case RenderCommandList::Primitive::LineStrip:      return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case RenderCommandList::Primitive::Triangle:       return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case RenderCommandList::Primitive::TriangleStrip:  return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    default:                                           META_UNEXPECTED_ARG_RETURN(primitive, D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);
    }
}

Ptr<RenderCommandList> RenderCommandList::Create(CommandQueue& cmd_queue, IRenderPass& render_pass)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderCommandListDX>(static_cast<CommandQueueBase&>(cmd_queue), static_cast<RenderPassBase&>(render_pass));
}

Ptr<RenderCommandList> RenderCommandList::Create(ParallelRenderCommandList& parallel_render_command_list)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderCommandListDX>(static_cast<ParallelRenderCommandListBase&>(parallel_render_command_list));
}

Ptr<RenderCommandList> RenderCommandListBase::CreateForSynchronization(CommandQueue& cmd_queue)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderCommandListDX>(static_cast<CommandQueueBase&>(cmd_queue));
}

RenderCommandListDX::RenderCommandListDX(CommandQueueBase& cmd_queue)
    : CommandListDX<RenderCommandListBase>(D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_queue)
{
    META_FUNCTION_TASK();
}

RenderCommandListDX::RenderCommandListDX(CommandQueueBase& cmd_queue, RenderPassBase& render_pass)
    : CommandListDX<RenderCommandListBase>(D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_queue, render_pass)
{
    META_FUNCTION_TASK();
}

RenderCommandListDX::RenderCommandListDX(ParallelRenderCommandListBase& parallel_render_command_list)
    : CommandListDX<RenderCommandListBase>(D3D12_COMMAND_LIST_TYPE_DIRECT, parallel_render_command_list)
{
    META_FUNCTION_TASK();
}

void RenderCommandListDX::ResetNative(const Ptr<RenderStateDX>& render_state_ptr)
{
    META_FUNCTION_TASK();
    if (!IsNativeCommitted())
        return;

    SetNativeCommitted(false);
    SetCommandListState(CommandList::State::Encoding);

    ID3D12PipelineState* p_dx_initial_state = render_state_ptr ? render_state_ptr->GetNativePipelineState().Get() : nullptr;
    ID3D12CommandAllocator& dx_cmd_allocator = GetNativeCommandAllocatorRef();
    ID3D12Device* p_native_device = GetCommandQueueDX().GetContextDX().GetDeviceDX().GetNativeDevice().Get();
    ThrowIfFailed(dx_cmd_allocator.Reset(), p_native_device);
    ThrowIfFailed(GetNativeCommandListRef().Reset(&dx_cmd_allocator, p_dx_initial_state), p_native_device);

    BeginGpuZone();

    if (!render_state_ptr)
        return;

    using namespace magic_enum::bitwise_operators;
    DrawingState& drawing_state = GetDrawingState();
    drawing_state.render_state_ptr    = render_state_ptr;
    drawing_state.render_state_groups = IRenderState::Groups::Program
                                      | IRenderState::Groups::Rasterizer
                                      | IRenderState::Groups::DepthStencil;
}

void RenderCommandListDX::ResetRenderPass()
{
    META_FUNCTION_TASK();
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

void RenderCommandListDX::Reset(DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    ResetNative();
    RenderCommandListBase::Reset(p_debug_group);
    if (HasPass())
    {
        ResetRenderPass();
    }
}

void RenderCommandListDX::ResetWithState(IRenderState& render_state, DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    ResetNative(static_cast<RenderStateBase&>(render_state).GetPtr<RenderStateDX>());
    RenderCommandListBase::ResetWithState(render_state, p_debug_group);
    if (HasPass())
    {
        ResetRenderPass();
    }
}

bool RenderCommandListDX::SetVertexBuffers(IBufferSet& vertex_buffers, bool set_resource_barriers)
{
    META_FUNCTION_TASK();
    if (!RenderCommandListBase::SetVertexBuffers(vertex_buffers, set_resource_barriers))
        return false;

    auto& dx_vertex_buffer_set = static_cast<BufferSetDX&>(vertex_buffers);
    if (const Ptr<IResourceBarriers>& buffer_set_setup_barriers_ptr = dx_vertex_buffer_set.GetSetupTransitionBarriers();
        set_resource_barriers && dx_vertex_buffer_set.SetState(IResource::State::VertexBuffer) && buffer_set_setup_barriers_ptr)
    {
        SetResourceBarriers(*buffer_set_setup_barriers_ptr);
    }

    const std::vector<D3D12_VERTEX_BUFFER_VIEW>& vertex_buffer_views = dx_vertex_buffer_set.GetNativeVertexBufferViews();
    GetNativeCommandListRef().IASetVertexBuffers(0, static_cast<UINT>(vertex_buffer_views.size()), vertex_buffer_views.data());
    return true;
}

bool RenderCommandListDX::SetIndexBuffer(IBuffer& index_buffer, bool set_resource_barriers)
{
    META_FUNCTION_TASK();
    if (!RenderCommandListBase::SetIndexBuffer(index_buffer, set_resource_barriers))
        return false;

    auto& dx_index_buffer = static_cast<IndexBufferDX&>(index_buffer);
    if (Ptr <IResourceBarriers>& buffer_setup_barriers_ptr = dx_index_buffer.GetSetupTransitionBarriers();
        set_resource_barriers && dx_index_buffer.SetState(IResource::State::IndexBuffer, buffer_setup_barriers_ptr) && buffer_setup_barriers_ptr)
    {
        SetResourceBarriers(*buffer_setup_barriers_ptr);
    }

    GetNativeCommandListRef().IASetIndexBuffer(&dx_index_buffer.GetNativeView());
    return true;
}

void RenderCommandListDX::DrawIndexed(Primitive primitive, uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                                      uint32_t instance_count, uint32_t start_instance)
{
    META_FUNCTION_TASK();

    DrawingState& drawing_state = GetDrawingState();
    if (index_count == 0 && drawing_state.index_buffer_ptr)
    {
        index_count = drawing_state.index_buffer_ptr->GetFormattedItemsCount();
    }

    RenderCommandListBase::DrawIndexed(primitive, index_count, start_index, start_vertex, instance_count, start_instance);

    using namespace magic_enum::bitwise_operators;
    ID3D12GraphicsCommandList& dx_command_list = GetNativeCommandListRef();
    if (static_cast<bool>(drawing_state.changes & DrawingState::Changes::PrimitiveType))
    {
        const D3D12_PRIMITIVE_TOPOLOGY primitive_topology = PrimitiveToDXTopology(primitive);
        dx_command_list.IASetPrimitiveTopology(primitive_topology);
        drawing_state.changes &= ~DrawingState::Changes::PrimitiveType;
    }

    dx_command_list.DrawIndexedInstanced(index_count, instance_count, start_index, start_vertex, start_instance);
}

void RenderCommandListDX::Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
                               uint32_t instance_count, uint32_t start_instance)
{
    META_FUNCTION_TASK();
    RenderCommandListBase::Draw(primitive, vertex_count, start_vertex, instance_count, start_instance);

    using namespace magic_enum::bitwise_operators;
    ID3D12GraphicsCommandList& dx_command_list = GetNativeCommandListRef();
    if (DrawingState& drawing_state = GetDrawingState();
        static_cast<bool>(drawing_state.changes & DrawingState::Changes::PrimitiveType))
    {
        const D3D12_PRIMITIVE_TOPOLOGY primitive_topology = PrimitiveToDXTopology(primitive);
        dx_command_list.IASetPrimitiveTopology(primitive_topology);
        drawing_state.changes &= ~DrawingState::Changes::PrimitiveType;
    }
    dx_command_list.DrawInstanced(vertex_count, instance_count, start_vertex, start_instance);
}

void RenderCommandListDX::Commit()
{
    META_FUNCTION_TASK();
    if (IsParallel())
    {
        CommandListDX<RenderCommandListBase>::Commit();
        return;
    }

    if (auto pass_dx = static_cast<RenderPassDX*>(GetPassPtr());
        pass_dx && pass_dx->IsBegun())
    {
        pass_dx->End(*this);
    }

    CommandListDX<RenderCommandListBase>::Commit();
}

RenderPassDX& RenderCommandListDX::GetPassDX()
{
    META_FUNCTION_TASK();
    return static_cast<RenderPassDX&>(GetPass());
}

} // namespace Methane::Graphics
