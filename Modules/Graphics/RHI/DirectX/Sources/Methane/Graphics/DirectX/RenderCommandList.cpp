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

FILE: Methane/Graphics/DirectX/RenderCommandList.cpp
DirectX 12 implementation of the render command list interface.

******************************************************************************/

#include <Methane/Graphics/DirectX/RenderCommandList.h>
#include <Methane/Graphics/DirectX/ParallelRenderCommandList.h>
#include <Methane/Graphics/DirectX/RenderState.h>
#include <Methane/Graphics/DirectX/RenderPass.h>
#include <Methane/Graphics/DirectX/CommandQueue.h>
#include <Methane/Graphics/DirectX/Device.h>
#include <Methane/Graphics/DirectX/Program.h>
#include <Methane/Graphics/DirectX/Buffer.h>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Instrumentation.h>
#include <Methane/Graphics/Windows/DirectXErrorHandling.h>

#include <magic_enum.hpp>
#include <directx/d3dx12.h>

namespace Methane::Graphics::Rhi
{

Ptr<IRenderCommandList> IRenderCommandList::Create(ICommandQueue& cmd_queue, IRenderPass& render_pass)
{
    META_FUNCTION_TASK();
    return std::make_shared<DirectX::RenderCommandList>(static_cast<Base::CommandQueue&>(cmd_queue), static_cast<Base::RenderPass&>(render_pass));
}

Ptr<IRenderCommandList> IRenderCommandList::Create(IParallelRenderCommandList& parallel_render_command_list)
{
    META_FUNCTION_TASK();
    return std::make_shared<DirectX::RenderCommandList>(static_cast<Base::ParallelRenderCommandList&>(parallel_render_command_list));
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Base
{

Ptr<Rhi::IRenderCommandList> RenderCommandList::CreateForSynchronization(Rhi::ICommandQueue& cmd_queue)
{
    META_FUNCTION_TASK();
    return std::make_shared<DirectX::RenderCommandList>(static_cast<Base::CommandQueue&>(cmd_queue));
}

} // namespace Methane::Graphics::Base

namespace Methane::Graphics::DirectX
{

static D3D12_PRIMITIVE_TOPOLOGY PrimitiveToDXTopology(Rhi::RenderPrimitive primitive)
{
    META_FUNCTION_TASK();
    switch (primitive)
    {
    case Rhi::RenderPrimitive::Point:          return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case Rhi::RenderPrimitive::Line:           return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case Rhi::RenderPrimitive::LineStrip:      return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case Rhi::RenderPrimitive::Triangle:       return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case Rhi::RenderPrimitive::TriangleStrip:  return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    default:                              META_UNEXPECTED_ARG_RETURN(primitive, D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);
    }
}

RenderCommandList::RenderCommandList(Base::CommandQueue& cmd_queue)
    : CommandList<Base::RenderCommandList>(D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_queue)
{
    META_FUNCTION_TASK();
}

RenderCommandList::RenderCommandList(Base::CommandQueue& cmd_queue, Base::RenderPass& render_pass)
    : CommandList<Base::RenderCommandList>(D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_queue, render_pass)
{
    META_FUNCTION_TASK();
}

RenderCommandList::RenderCommandList(Base::ParallelRenderCommandList& parallel_render_command_list)
    : CommandList<Base::RenderCommandList>(D3D12_COMMAND_LIST_TYPE_DIRECT, parallel_render_command_list)
{
    META_FUNCTION_TASK();
}

void RenderCommandList::ResetNative(const Ptr<RenderState>& render_state_ptr)
{
    META_FUNCTION_TASK();
    if (!IsNativeCommitted())
        return;

    SetNativeCommitted(false);
    SetCommandListState(Rhi::CommandListState::Encoding);

    ID3D12PipelineState* p_dx_initial_state = render_state_ptr ? render_state_ptr->GetNativePipelineState().Get() : nullptr;
    ID3D12CommandAllocator& dx_cmd_allocator = GetNativeCommandAllocatorRef();
    ID3D12Device* p_native_device = GetDirectCommandQueue().GetDirectContext().GetDirectDevice().GetNativeDevice().Get();
    ThrowIfFailed(dx_cmd_allocator.Reset(), p_native_device);
    ThrowIfFailed(GetNativeCommandListRef().Reset(&dx_cmd_allocator, p_dx_initial_state), p_native_device);

    BeginGpuZone();

    if (!render_state_ptr)
        return;

    DrawingState& drawing_state = GetDrawingState();
    drawing_state.render_state_ptr    = render_state_ptr;
    drawing_state.render_state_groups = Rhi::IRenderState::Groups({
        Rhi::RenderStateGroup::Program,
        Rhi::RenderStateGroup::Rasterizer,
        Rhi::RenderStateGroup::DepthStencil
    });
}

void RenderCommandList::ResetRenderPass()
{
    META_FUNCTION_TASK();
    RenderPass& pass_dx = GetDirectPass();

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

void RenderCommandList::Reset(IDebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    ResetNative();
    Base::RenderCommandList::Reset(p_debug_group);
    if (HasPass())
    {
        ResetRenderPass();
    }
}

void RenderCommandList::ResetWithState(Rhi::IRenderState& render_state, IDebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    ResetNative(static_cast<Base::RenderState&>(render_state).GetPtr<RenderState>());
    Base::RenderCommandList::ResetWithState(render_state, p_debug_group);
    if (HasPass())
    {
        ResetRenderPass();
    }
}

bool RenderCommandList::SetVertexBuffers(Rhi::IBufferSet& vertex_buffers, bool set_resource_barriers)
{
    META_FUNCTION_TASK();
    if (!Base::RenderCommandList::SetVertexBuffers(vertex_buffers, set_resource_barriers))
        return false;

    auto& dx_vertex_buffer_set = static_cast<BufferSet&>(vertex_buffers);
    if (const Ptr<Rhi::IResourceBarriers>& buffer_set_setup_barriers_ptr = dx_vertex_buffer_set.GetSetupTransitionBarriers();
        set_resource_barriers && dx_vertex_buffer_set.SetState(Rhi::ResourceState::VertexBuffer) && buffer_set_setup_barriers_ptr)
    {
        SetResourceBarriers(*buffer_set_setup_barriers_ptr);
    }

    const std::vector<D3D12_VERTEX_BUFFER_VIEW>& vertex_buffer_views = dx_vertex_buffer_set.GetNativeVertexBufferViews();
    GetNativeCommandListRef().IASetVertexBuffers(0, static_cast<UINT>(vertex_buffer_views.size()), vertex_buffer_views.data());
    return true;
}

bool RenderCommandList::SetIndexBuffer(Rhi::IBuffer& index_buffer, bool set_resource_barriers)
{
    META_FUNCTION_TASK();
    if (!Base::RenderCommandList::SetIndexBuffer(index_buffer, set_resource_barriers))
        return false;

    auto& dx_index_buffer = static_cast<IndexBuffer&>(index_buffer);
    if (Ptr<Rhi::IResourceBarriers>& buffer_setup_barriers_ptr = dx_index_buffer.GetSetupTransitionBarriers();
        set_resource_barriers && dx_index_buffer.SetState(Rhi::ResourceState::IndexBuffer, buffer_setup_barriers_ptr) && buffer_setup_barriers_ptr)
    {
        SetResourceBarriers(*buffer_setup_barriers_ptr);
    }

    GetNativeCommandListRef().IASetIndexBuffer(&dx_index_buffer.GetNativeView());
    return true;
}

void RenderCommandList::DrawIndexed(Primitive primitive, uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                                      uint32_t instance_count, uint32_t start_instance)
{
    META_FUNCTION_TASK();

    DrawingState& drawing_state = GetDrawingState();
    if (index_count == 0 && drawing_state.index_buffer_ptr)
    {
        index_count = drawing_state.index_buffer_ptr->GetFormattedItemsCount();
    }

    Base::RenderCommandList::DrawIndexed(primitive, index_count, start_index, start_vertex, instance_count, start_instance);

    ID3D12GraphicsCommandList& dx_command_list = GetNativeCommandListRef();
    if (static_cast<bool>(drawing_state.changes & DrawingState::Changes::PrimitiveType))
    {
        const D3D12_PRIMITIVE_TOPOLOGY primitive_topology = PrimitiveToDXTopology(primitive);
        dx_command_list.IASetPrimitiveTopology(primitive_topology);
        drawing_state.changes &= ~DrawingState::Changes::PrimitiveType;
    }

    dx_command_list.DrawIndexedInstanced(index_count, instance_count, start_index, start_vertex, start_instance);
}

void RenderCommandList::Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
                               uint32_t instance_count, uint32_t start_instance)
{
    META_FUNCTION_TASK();
    Base::RenderCommandList::Draw(primitive, vertex_count, start_vertex, instance_count, start_instance);

    using namespace magic_enumbitwise_operators;
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

void RenderCommandList::Commit()
{
    META_FUNCTION_TASK();
    if (IsParallel())
    {
        CommandList<Base::RenderCommandList>::Commit();
        return;
    }

    if (auto pass_dx = static_cast<RenderPass*>(GetPassPtr());
        pass_dx && pass_dx->IsBegun())
    {
        pass_dx->End(*this);
    }

    CommandList<Base::RenderCommandList>::Commit();
}

RenderPass& RenderCommandList::GetDirectPass()
{
    META_FUNCTION_TASK();
    return static_cast<RenderPass&>(GetPass());
}

} // namespace Methane::Graphics::DirectX
