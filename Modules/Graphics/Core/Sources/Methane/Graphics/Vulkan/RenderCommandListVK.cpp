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

FILE: Methane/Graphics/Vulkan/RenderCommandListVK.cpp
Vulkan implementation of the render command list interface.

******************************************************************************/

#include "RenderCommandListVK.h"
#include "ParallelRenderCommandListVK.h"
#include "RenderStateVK.h"
#include "RenderPassVK.h"
#include "CommandQueueVK.h"
#include "ContextVK.h"
#include "BufferVK.h"

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>

namespace Methane::Graphics
{

vk::PrimitiveTopology GetVulkanPrimitiveTopology(RenderCommandList::Primitive primitive_type)
{
    META_FUNCTION_TASK();
    switch(primitive_type)
    {
    case RenderCommandList::Primitive::Point:           return vk::PrimitiveTopology::ePointList;
    case RenderCommandList::Primitive::Line:            return vk::PrimitiveTopology::eLineList;
    case RenderCommandList::Primitive::LineStrip:       return vk::PrimitiveTopology::eLineStrip;
    case RenderCommandList::Primitive::Triangle:        return vk::PrimitiveTopology::eTriangleList;
    case RenderCommandList::Primitive::TriangleStrip:   return vk::PrimitiveTopology::eTriangleStrip;
    default: META_UNEXPECTED_ARG_RETURN(primitive_type, vk::PrimitiveTopology::ePointList);
    }
}

static vk::IndexType GetVulkanIndexTypeByStride(Data::Size index_stride_bytes)
{
    META_FUNCTION_TASK();
    switch(index_stride_bytes)
    {
    case 1: return vk::IndexType::eUint8EXT;
    case 2: return vk::IndexType::eUint16;
    case 4: return vk::IndexType::eUint32;
    default: META_UNEXPECTED_ARG_DESCR_RETURN(index_stride_bytes, vk::IndexType::eNoneKHR, "unsupported index buffer stride size");
    }
}

static vk::CommandBufferInheritanceInfo CreateRenderCommandBufferInheritanceInfo(const RenderPassVK& render_pass) noexcept
{
    META_FUNCTION_TASK();
    return vk::CommandBufferInheritanceInfo(
        render_pass.GetPatternVK().GetNativeRenderPass(),
        0U, // sub-pass
        render_pass.GetNativeFrameBuffer()
    );
}

Ptr<RenderCommandList> RenderCommandList::Create(CommandQueue& command_queue, RenderPass& render_pass)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderCommandListVK>(static_cast<CommandQueueVK&>(command_queue), static_cast<RenderPassVK&>(render_pass));
}

Ptr<RenderCommandList> RenderCommandList::Create(ParallelRenderCommandList& parallel_render_command_list)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderCommandListVK>(static_cast<ParallelRenderCommandListVK&>(parallel_render_command_list), false);
}

Ptr<RenderCommandList> RenderCommandListBase::CreateForSynchronization(CommandQueue& cmd_queue)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderCommandListVK>(static_cast<CommandQueueVK&>(cmd_queue));
}

RenderCommandListVK::RenderCommandListVK(CommandQueueVK& command_queue)
    : CommandListVK(vk::CommandBufferInheritanceInfo(), command_queue)
{
    META_FUNCTION_TASK();
}

RenderCommandListVK::RenderCommandListVK(CommandQueueVK& command_queue, RenderPassVK& render_pass)
    : CommandListVK(CreateRenderCommandBufferInheritanceInfo(render_pass), command_queue, render_pass)
{
    META_FUNCTION_TASK();
    static_cast<Data::IEmitter<IRenderPassCallback>&>(render_pass).Connect(*this);
}

RenderCommandListVK::RenderCommandListVK(ParallelRenderCommandListVK& parallel_render_command_list, bool is_beginning_cmd_list)
    : CommandListVK(CreateRenderCommandBufferInheritanceInfo(parallel_render_command_list.GetPassVK()), parallel_render_command_list, is_beginning_cmd_list)
{
    META_FUNCTION_TASK();
    static_cast<Data::IEmitter<IRenderPassCallback>&>(parallel_render_command_list.GetPassVK()).Connect(*this);
}

void RenderCommandListVK::Reset(DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    CommandListVK::ResetCommandState();
    CommandListVK::Reset(p_debug_group);
}

void RenderCommandListVK::ResetWithState(IRenderState& render_state, DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    CommandListVK::ResetCommandState();
    CommandListVK::Reset(p_debug_group);
    CommandListVK::SetRenderState(render_state);
}

bool RenderCommandListVK::SetVertexBuffers(BufferSet& vertex_buffers, bool set_resource_barriers)
{
    META_FUNCTION_TASK();
    if (!RenderCommandListBase::SetVertexBuffers(vertex_buffers, set_resource_barriers))
        return false;

    const auto& vk_vertex_buffers = static_cast<const BufferSetVK&>(vertex_buffers);
    auto& vk_vertex_buffer_set = static_cast<BufferSetVK&>(vertex_buffers);
    if (const Ptr<Resource::Barriers>& buffer_set_setup_barriers_ptr = vk_vertex_buffer_set.GetSetupTransitionBarriers();
        set_resource_barriers && vk_vertex_buffer_set.SetState(Resource::State::VertexBuffer) && buffer_set_setup_barriers_ptr)
    {
        SetResourceBarriers(*buffer_set_setup_barriers_ptr);
    }

    GetNativeCommandBufferDefault().bindVertexBuffers(0U, vk_vertex_buffers.GetNativeBuffers(), vk_vertex_buffers.GetNativeOffsets());
    return true;
}

bool RenderCommandListVK::SetIndexBuffer(Buffer& index_buffer, bool set_resource_barriers)
{
    META_FUNCTION_TASK();
    if (!RenderCommandListBase::SetIndexBuffer(index_buffer, set_resource_barriers))
        return false;

    auto& vk_index_buffer = static_cast<BufferVK&>(index_buffer);
    if (Ptr<Resource::Barriers>& buffer_setup_barriers_ptr = vk_index_buffer.GetSetupTransitionBarriers();
        set_resource_barriers && vk_index_buffer.SetState(Resource::State::IndexBuffer, buffer_setup_barriers_ptr) && buffer_setup_barriers_ptr)
    {
        SetResourceBarriers(*buffer_setup_barriers_ptr);
    }

    const vk::IndexType vk_index_type = GetVulkanIndexTypeByStride(index_buffer.GetSettings().item_stride_size);
    GetNativeCommandBufferDefault().bindIndexBuffer(vk_index_buffer.GetNativeResource(), 0U, vk_index_type);
    return true;
}

void RenderCommandListVK::DrawIndexed(Primitive primitive, uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                                      uint32_t instance_count, uint32_t start_instance)
{
    META_FUNCTION_TASK();
    if (const DrawingState& drawing_state = GetDrawingState();
        index_count == 0 && drawing_state.index_buffer_ptr)
    {
        index_count = drawing_state.index_buffer_ptr->GetFormattedItemsCount();
    }

    RenderCommandListBase::DrawIndexed(primitive, index_count, start_index, start_vertex, instance_count, start_instance);

    UpdatePrimitiveTopology(primitive);
    GetNativeCommandBufferDefault().drawIndexed(index_count, instance_count, start_index, start_vertex, start_instance);
}

void RenderCommandListVK::Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
                               uint32_t instance_count, uint32_t start_instance)
{
    META_FUNCTION_TASK();
    RenderCommandListBase::Draw(primitive, vertex_count, start_vertex, instance_count, start_instance);

    UpdatePrimitiveTopology(primitive);
    GetNativeCommandBufferDefault().draw(vertex_count, instance_count, start_vertex, start_instance);
}

void RenderCommandListVK::Commit()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_FALSE(IsCommitted());

    if (!IsParallel())
    {
        CommitCommandBuffer(CommandBufferType::SecondaryRenderPass);

        auto render_pass_ptr = static_cast<RenderPassVK*>(GetPassPtr());
        if (render_pass_ptr)
            render_pass_ptr->Begin(*this);

        GetNativeCommandBuffer(CommandBufferType::Primary).executeCommands(GetNativeCommandBuffer(CommandBufferType::SecondaryRenderPass));

        if (render_pass_ptr)
            render_pass_ptr->End(*this);
    }

    CommandListVK::Commit();
}

void RenderCommandListVK::OnRenderPassUpdated(const RenderPass& render_pass)
{
    META_FUNCTION_TASK();
    SetSecondaryRenderBufferInheritInfo(CreateRenderCommandBufferInheritanceInfo(static_cast<const RenderPassVK&>(render_pass)));
}

void RenderCommandListVK::UpdatePrimitiveTopology(Primitive primitive)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;
    if (DrawingState& drawing_state = GetDrawingState();
        static_cast<bool>(drawing_state.changes & DrawingState::Changes::PrimitiveType))
    {
        const vk::PrimitiveTopology vk_primitive_topology = GetVulkanPrimitiveTopology(primitive);
        GetNativeCommandBufferDefault().setPrimitiveTopologyEXT(vk_primitive_topology);
        drawing_state.changes &= ~DrawingState::Changes::PrimitiveType;
    }
}

RenderPassVK& RenderCommandListVK::GetPassVK()
{
    META_FUNCTION_TASK();
    return static_cast<class RenderPassVK&>(GetPass());
}

} // namespace Methane::Graphics
