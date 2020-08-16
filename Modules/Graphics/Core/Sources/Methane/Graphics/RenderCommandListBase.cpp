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

FILE: Methane/Graphics/RenderCommandListBase.cpp
Base implementation of the render command list interface.

******************************************************************************/

#include "RenderCommandListBase.h"
#include "ParallelRenderCommandListBase.h"
#include "CommandQueueBase.h"
#include "RenderPassBase.h"
#include "RenderStateBase.h"
#include "BufferBase.h"
#include "ProgramBase.h"

#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

RenderCommandListBase::RenderCommandListBase(CommandQueueBase& command_queue, RenderPassBase& pass)
    : CommandListBase(command_queue, Type::Render)
    , m_is_parallel(false)
    , m_sp_render_pass(pass.GetRenderPassPtr())
{
    META_FUNCTION_TASK();
}

RenderCommandListBase::RenderCommandListBase(ParallelRenderCommandListBase& parallel_render_command_list)
    : CommandListBase(static_cast<CommandQueueBase&>(parallel_render_command_list.GetCommandQueue()), Type::Render)
    , m_is_parallel(true)
    , m_sp_render_pass(parallel_render_command_list.GetPass().GetRenderPassPtr())
    , m_wp_parallel_render_command_list(parallel_render_command_list.GetParallelRenderCommandListPtr())
{
    META_FUNCTION_TASK();
}

void RenderCommandListBase::Reset(const Ptr<RenderState>& sp_render_state, DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();

    CommandListBase::Reset(p_debug_group);

    // ResetCommandState() must be called from the top-most overridden Reset method

    if (sp_render_state)
    {
        SetState(*sp_render_state);
    }
}

void RenderCommandListBase::SetState(RenderState& render_state, RenderState::Group::Mask state_groups)
{
    META_FUNCTION_TASK();
    VerifyEncodingState();

    DrawingState& drawing_state = GetDrawingState();
    const RenderState::Group::Mask changed_states = (drawing_state.p_render_state
                                                  ? RenderState::Settings::Compare(render_state.GetSettings(),
                                                      drawing_state.p_render_state->GetSettings(),
                                                      drawing_state.render_state_groups)
                                                  : RenderState::Group::All)
                                                  | ~drawing_state.render_state_groups;

    RenderStateBase& render_state_base = static_cast<RenderStateBase&>(render_state);
    render_state_base.Apply(*this, changed_states & state_groups);

    drawing_state.p_render_state = &render_state_base;
    drawing_state.render_state_groups |= state_groups;
}

void RenderCommandListBase::SetVertexBuffers(const BufferSet& vertex_buffers)
{
    META_FUNCTION_TASK();
    VerifyEncodingState();

    if (vertex_buffers.GetType() != Buffer::Type::Vertex)
    {
        throw std::invalid_argument("Can not set buffers of \"" + Buffer::GetBufferTypeName(vertex_buffers.GetType()) +
                                    "\" type where \"Vertex\" buffers are required.");
    }

    const BufferSetBase& vertex_buffers_base = static_cast<const BufferSetBase&>(vertex_buffers);
    DrawingState       & drawing_state       = GetDrawingState();
    if (drawing_state.vertex_buffers != vertex_buffers_base.GetRawPtrs())
        drawing_state.changes |= DrawingState::Changes::VertexBuffers;

    drawing_state.vertex_buffers = vertex_buffers_base.GetRawPtrs();
}

void RenderCommandListBase::DrawIndexed(Primitive primitive_type, Buffer& index_buffer,
                                        uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                                        uint32_t instance_count, uint32_t /*start_instance*/)
{
    META_FUNCTION_TASK();
    VerifyEncodingState();

    if (index_buffer.GetSettings().type != Buffer::Type::Index)
    {
        throw std::invalid_argument("Can not draw with index buffer of wrong type \"" + 
                                    static_cast<const BufferBase&>(index_buffer).GetBufferTypeName() +
                                    "\". Buffer of \"Index\" type is expected.");
    }

    const uint32_t formatted_items_count = index_buffer.GetFormattedItemsCount();
    if (formatted_items_count == 0)
    {
        throw std::invalid_argument("Can not draw with index buffer which contains no formatted vertices.");
    }
    if (index_count == 0)
    {
        throw std::invalid_argument("Can not draw zero index/vertex count.");
    }
    if (start_index + index_count > formatted_items_count)
    {
        throw std::invalid_argument("Ending index is out of buffer bounds.");
    }
    if (instance_count == 0)
    {
        throw std::invalid_argument("Can not draw zero instances.");
    }

    ValidateDrawVertexBuffers(start_vertex);

    DrawingState& drawing_state = GetDrawingState();
    if (!drawing_state.p_index_buffer || drawing_state.p_index_buffer != std::addressof(index_buffer))
        drawing_state.changes |= DrawingState::Changes::IndexBuffer;
    drawing_state.p_index_buffer = static_cast<BufferBase*>(&index_buffer);

    if (!drawing_state.opt_primitive_type || *drawing_state.opt_primitive_type != primitive_type)
        drawing_state.changes |= DrawingState::Changes::PrimitiveType;
    drawing_state.opt_primitive_type = primitive_type;
}

void RenderCommandListBase::Draw(Primitive primitive_type, uint32_t vertex_count, uint32_t start_vertex,
                                 uint32_t instance_count, uint32_t)
{
    META_FUNCTION_TASK();
    VerifyEncodingState();

    if (vertex_count == 0)
    {
        throw std::invalid_argument("Can not draw zero vertices.");
    }
    if (instance_count == 0)
    {
        throw std::invalid_argument("Can not draw zero instances.");
    }

    ValidateDrawVertexBuffers(start_vertex, vertex_count);

    DrawingState& drawing_state = GetDrawingState();
    if (!drawing_state.opt_primitive_type || *drawing_state.opt_primitive_type != primitive_type)
        drawing_state.changes |= DrawingState::Changes::PrimitiveType;
    drawing_state.opt_primitive_type = primitive_type;
}

void RenderCommandListBase::ResetCommandState()
{
    META_FUNCTION_TASK();
    CommandListBase::ResetCommandState();

    m_drawing_state.opt_primitive_type.reset();
    m_drawing_state.p_index_buffer = nullptr;
    std::fill(m_drawing_state.vertex_buffers.begin(), m_drawing_state.vertex_buffers.end(), nullptr);
    m_drawing_state.p_render_state = nullptr;
    m_drawing_state.render_state_groups = RenderState::Group::None;
    m_drawing_state.changes = DrawingState::Changes::None;
}

void RenderCommandListBase::ValidateDrawVertexBuffers(uint32_t draw_start_vertex, uint32_t draw_vertex_count)
{
    META_FUNCTION_TASK();

    DrawingState& drawing_state = GetDrawingState();
    for (BufferBase* p_vertex_buffer : drawing_state.vertex_buffers)
    {
        assert(!!p_vertex_buffer);
        const BufferBase& vertex_buffer = *p_vertex_buffer;
        const uint32_t    vertex_count  = vertex_buffer.GetFormattedItemsCount();
        if (draw_start_vertex + draw_vertex_count > vertex_count)
        {
            throw std::invalid_argument("Can not draw starting from vertex " + std::to_string(draw_start_vertex) +
                                        (draw_vertex_count ? " with " + std::to_string(draw_vertex_count) + " vertex count " : "") +
                                        " which is out of bound for vertex buffer \"" + vertex_buffer.GetName() +
                                        "\" (size " + std::to_string(vertex_count) + ").");
        }
    }
}

RenderPassBase& RenderCommandListBase::GetPass()
{
    META_FUNCTION_TASK();
    assert(!!m_sp_render_pass);
    return static_cast<RenderPassBase&>(*m_sp_render_pass);
}

} // namespace Methane::Graphics
