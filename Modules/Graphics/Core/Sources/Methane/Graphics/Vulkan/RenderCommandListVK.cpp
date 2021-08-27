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

Ptr<RenderCommandList> RenderCommandList::Create(CommandQueue& command_queue, RenderPass& render_pass)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderCommandListVK>(static_cast<CommandQueueVK&>(command_queue), static_cast<RenderPassVK&>(render_pass));
}

Ptr<RenderCommandList> RenderCommandList::Create(ParallelRenderCommandList& parallel_render_command_list)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderCommandListVK>(static_cast<ParallelRenderCommandListVK&>(parallel_render_command_list));
}

Ptr<RenderCommandList> RenderCommandListBase::CreateForSynchronization(CommandQueue&)
{
    META_FUNCTION_TASK();
    return nullptr;
}

RenderCommandListVK::RenderCommandListVK(CommandQueueVK& command_queue, RenderPassVK& render_pass)
    : CommandListVK<RenderCommandListBase>(command_queue, render_pass)
{
    META_FUNCTION_TASK();
}

RenderCommandListVK::RenderCommandListVK(ParallelRenderCommandListVK& parallel_render_command_list)
    : CommandListVK<RenderCommandListBase>(parallel_render_command_list)
{
    META_FUNCTION_TASK();
}

void RenderCommandListVK::Reset(DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    CommandListVK<RenderCommandListBase>::ResetCommandState();
    CommandListVK<RenderCommandListBase>::Reset(p_debug_group);
    if (HasPass())
    {
        ResetRenderPass();
    }
}

void RenderCommandListVK::ResetWithState(RenderState& render_state, DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    CommandListVK<RenderCommandListBase>::ResetCommandState();
    CommandListVK<RenderCommandListBase>::Reset(p_debug_group);
    CommandListVK<RenderCommandListBase>::SetRenderState(render_state);
    if (HasPass())
    {
        ResetRenderPass();
    }
}

bool RenderCommandListVK::SetVertexBuffers(BufferSet& vertex_buffers, bool set_resource_barriers)
{
    META_FUNCTION_TASK();
    if (!RenderCommandListBase::SetVertexBuffers(vertex_buffers, set_resource_barriers))
        return false;

    auto& vk_vertex_buffers = static_cast<BufferSetVK&>(vertex_buffers);
    GetNativeCommandBuffer().bindVertexBuffers(0U, vk_vertex_buffers.GetNativeBuffers(), vk_vertex_buffers.GetNativeOffsets());
    return true;
}

bool RenderCommandListVK::SetIndexBuffer(Buffer& index_buffer, bool set_resource_barriers)
{
    META_FUNCTION_TASK();
    if (!RenderCommandListBase::SetIndexBuffer(index_buffer, set_resource_barriers))
        return false;


    auto& vk_index_buffer = static_cast<BufferVK&>(index_buffer);
    const vk::IndexType vk_index_type = GetVulkanIndexTypeByStride(index_buffer.GetSettings().item_stride_size);
    GetNativeCommandBuffer().bindIndexBuffer(vk_index_buffer.GetNativeBuffer(), 0U, vk_index_type);
    return true;
}

void RenderCommandListVK::DrawIndexed(Primitive primitive, uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                                      uint32_t instance_count, uint32_t start_instance)
{
    META_FUNCTION_TASK();

    DrawingState& drawing_state = GetDrawingState();
    if (index_count == 0 && drawing_state.index_buffer_ptr)
    {
        index_count = drawing_state.index_buffer_ptr->GetFormattedItemsCount();
    }

    RenderCommandListBase::DrawIndexed(primitive, index_count, start_index, start_vertex, instance_count, start_instance);

    UpdatePrimitiveTopology(primitive);
    GetNativeCommandBuffer().drawIndexed(index_count, instance_count, start_index, start_vertex, start_instance);
}

void RenderCommandListVK::Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
                               uint32_t instance_count, uint32_t start_instance)
{
    META_FUNCTION_TASK();
    RenderCommandListBase::Draw(primitive, vertex_count, start_vertex, instance_count, start_instance);

    UpdatePrimitiveTopology(primitive);
    GetNativeCommandBuffer().draw(vertex_count, instance_count, start_vertex, start_instance);
}

void RenderCommandListVK::Commit()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_FALSE(IsCommitted());

    if (auto pass_vk = static_cast<RenderPassVK*>(GetPassPtr());
        pass_vk && pass_vk->IsBegun())
    {
        pass_vk->End(*this);
    }

    CommandListVK<RenderCommandListBase>::Commit();
}

void RenderCommandListVK::Execute(uint32_t frame_index, const CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    RenderCommandListBase::Execute(frame_index, completed_callback);
}

void RenderCommandListVK::ResetRenderPass()
{
    META_FUNCTION_TASK();
    RenderPassVK& pass_vk = GetPassVK();

    if (IsParallel())
    {
        // TODO: support parallel render command lists
    }
    else if (!pass_vk.IsBegun())
    {
        pass_vk.Begin(*this);
    }
}

void RenderCommandListVK::UpdatePrimitiveTopology(Primitive primitive)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;
    if (DrawingState& drawing_state = GetDrawingState();
        magic_enum::flags::enum_contains(drawing_state.changes & DrawingState::Changes::PrimitiveType))
    {
        const vk::PrimitiveTopology vk_primitive_topology = GetVulkanPrimitiveTopology(primitive);
        GetNativeCommandBuffer().setPrimitiveTopologyEXT(vk_primitive_topology);
        drawing_state.changes &= ~DrawingState::Changes::PrimitiveType;
    }
}

RenderPassVK& RenderCommandListVK::GetPassVK()
{
    META_FUNCTION_TASK();
    return static_cast<class RenderPassVK&>(GetPass());
}

} // namespace Methane::Graphics
