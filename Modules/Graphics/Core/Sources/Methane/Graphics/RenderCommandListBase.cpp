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

FILE: Methane/Graphics/RenderCommandListBase.cpp
Base implementation of the render command list interface.

******************************************************************************/

#include "RenderCommandListBase.h"
#include "RenderPassBase.h"
#include "RenderStateBase.h"
#include "BufferBase.h"

#include <Methane/Data/Instrumentation.h>

#include <cassert>

#ifdef DrawState
#undef DrawState
#endif

namespace Methane::Graphics
{

RenderCommandListBase::RenderCommandListBase(CommandQueueBase& command_queue, RenderPassBase& pass)
    : CommandListBase(command_queue)
    , m_sp_pass(pass.GetPtr())
{
    ITT_FUNCTION_TASK();
}

void RenderCommandListBase::State::Reset()
{
    opt_primitive_type.reset();
    sp_index_buffer.reset();
    sp_vertex_buffers.clear();

    flags = { };
}

void RenderCommandListBase::Reset(RenderState& render_state, const std::string& debug_group)
{
    ITT_FUNCTION_TASK();

    m_draw_state.Reset();

    if (m_debug_group_opened)
    {
        PopDebugGroup();
        m_debug_group_opened = false;
    }

    if (!debug_group.empty())
    {
        PushDebugGroup(debug_group);
        m_debug_group_opened = true;
    }
    
    static_cast<RenderStateBase&>(render_state).Apply(*this);
}

void RenderCommandListBase::SetVertexBuffers(const Buffer::Refs& vertex_buffers)
{
    ITT_FUNCTION_TASK();
    if (vertex_buffers.empty())
    {
        throw std::invalid_argument("Can not set empty vertex buffers.");
    }

    m_draw_state.flags.vertex_buffers_changed = m_draw_state.sp_vertex_buffers.size() != vertex_buffers.size();
    m_draw_state.sp_vertex_buffers.resize(vertex_buffers.size());

    uint32_t vertex_buffer_index = 0;
    for (const Buffer::Ref& vertex_buffer_ref : vertex_buffers)
    {
        BufferBase& vertex_buffer = static_cast<BufferBase&>(vertex_buffer_ref.get());

        if (vertex_buffer.GetBufferType() != Buffer::Type::Vertex)
        {
            throw std::invalid_argument("Can not set vertex buffer \"" + vertex_buffer.GetName() +
                                        "\" of wrong type \"" + static_cast<const BufferBase&>(vertex_buffer).GetBufferTypeName() +
                                        "\". Buffer of \"Vertex\" type is expected.");
        }
        if (!vertex_buffer.GetDataSize())
        {
            throw std::invalid_argument("Can not set empty vertex buffer.");
        }

        if (!m_draw_state.flags.vertex_buffers_changed &&
            (vertex_buffer_index >= m_draw_state.sp_vertex_buffers.size() ||
            !m_draw_state.sp_vertex_buffers[vertex_buffer_index] ||
             m_draw_state.sp_vertex_buffers[vertex_buffer_index].get() != std::addressof(vertex_buffer)))
        {
            m_draw_state.flags.vertex_buffers_changed = true;
        }

        m_draw_state.sp_vertex_buffers[vertex_buffer_index] = vertex_buffer.GetPtr();
        vertex_buffer_index++;
    }
}

void RenderCommandListBase::DrawIndexed(Primitive primitive_type, Buffer& index_buffer,
                                        uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                                        uint32_t instance_count, uint32_t /*start_instance*/)
{
    ITT_FUNCTION_TASK();

    if (index_buffer.GetBufferType() != Buffer::Type::Index)
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

    m_draw_state.flags.index_buffer_changed = !m_draw_state.sp_index_buffer || m_draw_state.sp_index_buffer.get() != std::addressof(index_buffer);
    m_draw_state.sp_index_buffer = static_cast<BufferBase&>(index_buffer).GetPtr();

    m_draw_state.flags.primitive_type_changed = !m_draw_state.opt_primitive_type || *m_draw_state.opt_primitive_type != primitive_type;
    m_draw_state.opt_primitive_type = primitive_type;
}

void RenderCommandListBase::Draw(Primitive primitive_type, uint32_t vertex_count, uint32_t start_vertex,
                                 uint32_t instance_count, uint32_t start_instance)
{
    ITT_FUNCTION_TASK();

    if (vertex_count == 0)
    {
        throw std::invalid_argument("Can not draw zero vertices.");
    }
    if (instance_count == 0)
    {
        throw std::invalid_argument("Can not draw zero instances.");
    }

    ValidateDrawVertexBuffers(start_vertex, vertex_count);

    m_draw_state.flags.primitive_type_changed = !m_draw_state.opt_primitive_type || *m_draw_state.opt_primitive_type != primitive_type;
    m_draw_state.opt_primitive_type = primitive_type;
}

void RenderCommandListBase::ValidateDrawVertexBuffers(uint32_t draw_start_vertex, uint32_t draw_vertex_count)
{
    for (const BufferBase::Ptr& sp_vertex_buffer : m_draw_state.sp_vertex_buffers)
    {
        assert(!!sp_vertex_buffer);
        const BufferBase& vertex_buffer = *sp_vertex_buffer;
        const uint32_t    vertex_count  = vertex_buffer.GetFormattedItemsCount();
        if (draw_start_vertex + draw_vertex_count <= vertex_count)
            return;

        throw std::invalid_argument("Can not draw starting from vertex " + std::to_string(draw_start_vertex) +
                                    (draw_vertex_count ? " with " + std::to_string(draw_vertex_count) + " vertex count " : "") +
                                    " which is out of bound for vertex buffer \"" + vertex_buffer.GetName() +
                                    "\" (size " + std::to_string(vertex_count) + ").");
    }
}


RenderPassBase& RenderCommandListBase::GetPass()
{
    ITT_FUNCTION_TASK();
    assert(!!m_sp_pass);
    return static_cast<RenderPassBase&>(*m_sp_pass);
}

} // namespace Methane::Graphics
