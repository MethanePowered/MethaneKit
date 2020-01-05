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

FILE: Methane/Graphics/Vulkan/RenderCommandListVK.mm
Vulkan implementation of the render command list interface.

******************************************************************************/

#include "RenderCommandListVK.h"
#include "ParallelRenderCommandListVK.h"
#include "RenderStateVK.h"
#include "RenderPassVK.h"
#include "CommandQueueVK.h"
#include "ContextVK.h"
#include "BufferVK.h"

#include <Methane/Data/Instrumentation.h>

namespace Methane::Graphics
{

RenderCommandList::Ptr RenderCommandList::Create(CommandQueue& command_queue, RenderPass& render_pass)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<RenderCommandListVK>(static_cast<CommandQueueBase&>(command_queue), static_cast<RenderPassBase&>(render_pass));
}

RenderCommandList::Ptr RenderCommandList::Create(ParallelRenderCommandList& parallel_render_command_list)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<RenderCommandListVK>(static_cast<ParallelRenderCommandListBase&>(parallel_render_command_list));
}

RenderCommandListVK::RenderCommandListVK(CommandQueueBase& command_queue, RenderPassBase& render_pass)
    : RenderCommandListBase(command_queue, render_pass)
{
    ITT_FUNCTION_TASK();
}

RenderCommandListVK::RenderCommandListVK(ParallelRenderCommandListBase& parallel_render_command_list)
    : RenderCommandListBase(parallel_render_command_list)
{
    ITT_FUNCTION_TASK();
}

void RenderCommandListVK::Reset(RenderState& render_state, const std::string& debug_group)
{
    ITT_FUNCTION_TASK();

    RenderCommandListBase::ResetDrawState();
    RenderCommandListBase::Reset(render_state, debug_group);
}

void RenderCommandListVK::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    RenderCommandListBase::SetName(name);
}

void RenderCommandListVK::PushDebugGroup(const std::string& name)
{
    ITT_FUNCTION_TASK();
}

void RenderCommandListVK::PopDebugGroup()
{
    ITT_FUNCTION_TASK();
}

void RenderCommandListVK::SetVertexBuffers(const Buffer::Refs& vertex_buffers)
{
    ITT_FUNCTION_TASK();

    RenderCommandListBase::SetVertexBuffers(vertex_buffers);

    if (!m_draw_state.flags.vertex_buffers_changed)
        return;

    uint32_t vb_index = 0;
    for (auto vertex_buffer_ref : vertex_buffers)
    {
        assert(vertex_buffer_ref.get().GetBufferType() == Buffer::Type::Vertex);
        const BufferVK& vulkan_buffer = static_cast<const BufferVK&>(vertex_buffer_ref.get());
        vb_index++;
    }
}

void RenderCommandListVK::DrawIndexed(Primitive primitive, Buffer& index_buffer,
                                      uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                                      uint32_t instance_count, uint32_t start_instance)
{
    ITT_FUNCTION_TASK();
    
    const BufferVK& vulkan_index_buffer = static_cast<const BufferVK&>(index_buffer);
    if (index_count == 0)
    {
        index_count = vulkan_index_buffer.GetFormattedItemsCount();
    }

    RenderCommandListBase::DrawIndexed(primitive, index_buffer, index_count, start_index, start_vertex, instance_count, start_instance);
}

void RenderCommandListVK::Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
                               uint32_t instance_count, uint32_t start_instance)
{
    ITT_FUNCTION_TASK();

    RenderCommandListBase::Draw(primitive, vertex_count, start_vertex, instance_count, start_instance);
}

void RenderCommandListVK::Commit(bool present_drawable)
{
    ITT_FUNCTION_TASK();
    
    assert(!IsCommitted());

    RenderCommandListBase::Commit(present_drawable);
}

void RenderCommandListVK::Execute(uint32_t frame_index)
{
    ITT_FUNCTION_TASK();

    RenderCommandListBase::Execute(frame_index);
}

CommandQueueVK& RenderCommandListVK::GetCommandQueueVK() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<class CommandQueueVK&>(*m_sp_command_queue);
}

RenderPassVK& RenderCommandListVK::GetPassVK()
{
    ITT_FUNCTION_TASK();
    return static_cast<class RenderPassVK&>(GetPass());
}

} // namespace Methane::Graphics
