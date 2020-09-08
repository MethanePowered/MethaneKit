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
#include "TextureBase.h"

#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

RenderCommandListBase::RenderCommandListBase(CommandQueueBase& command_queue, RenderPassBase& pass)
    : CommandListBase(command_queue, Type::Render)
    , m_is_parallel(false)
    , m_render_pass_ptr(pass.GetRenderPassPtr())
{
    META_FUNCTION_TASK();
}

RenderCommandListBase::RenderCommandListBase(ParallelRenderCommandListBase& parallel_render_command_list)
    : CommandListBase(static_cast<CommandQueueBase&>(parallel_render_command_list.GetCommandQueue()), Type::Render)
    , m_is_parallel(true)
    , m_render_pass_ptr(parallel_render_command_list.GetPass().GetRenderPassPtr())
    , m_parallel_render_command_list_wptr(parallel_render_command_list.GetParallelRenderCommandListPtr())
{
    META_FUNCTION_TASK();
}

void RenderCommandListBase::Reset(const Ptr<RenderState>& render_state_ptr, DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();

    CommandListBase::Reset(p_debug_group);

    m_drawing_state.render_pass_attachments_ptr = m_render_pass_ptr->GetNonFrameBufferAttachmentTextures();

    if (render_state_ptr)
    {
        SetRenderState(*render_state_ptr);
    }
}

void RenderCommandListBase::SetRenderState(RenderState& render_state, RenderState::Group::Mask state_groups)
{
    META_FUNCTION_TASK();

    VerifyEncodingState();

    const bool render_state_changed = m_drawing_state.render_state_ptr.get() != std::addressof(render_state);
    const RenderState::Group::Mask changed_states = (m_drawing_state.render_state_ptr && render_state_changed
                                                  ? RenderState::Settings::Compare(render_state.GetSettings(),
                                                                                   m_drawing_state.render_state_ptr->GetSettings(),
                                                                                   m_drawing_state.render_state_groups)
                                                  : (m_drawing_state.render_state_ptr ? RenderState::Group::None
                                                                                     : RenderState::Group::All))
                                                  | ~m_drawing_state.render_state_groups;

    RenderStateBase& render_state_base = static_cast<RenderStateBase&>(render_state);
    render_state_base.Apply(*this, changed_states & state_groups);

    Ptr<ObjectBase> render_state_object_ptr = render_state_base.GetBasePtr();
    m_drawing_state.render_state_ptr = std::static_pointer_cast<RenderStateBase>(render_state_object_ptr);
    m_drawing_state.render_state_groups |= state_groups;

    if (render_state_changed)
    {
        RetainResource(std::move(render_state_object_ptr));
    }
}

void RenderCommandListBase::SetViewState(ViewState& view_state)
{
    META_FUNCTION_TASK();
    VerifyEncodingState();

    DrawingState& drawing_state = GetDrawingState();
    ViewStateBase* p_prev_view_state = drawing_state.p_view_state;
    drawing_state.p_view_state = static_cast<ViewStateBase*>(&view_state);

    if (p_prev_view_state && p_prev_view_state->GetSettings() == view_state.GetSettings())
        return;

    drawing_state.p_view_state->Apply(*this);
}

void RenderCommandListBase::SetVertexBuffers(BufferSet& vertex_buffers)
{
    META_FUNCTION_TASK();
    VerifyEncodingState();

    if (vertex_buffers.GetType() != Buffer::Type::Vertex)
    {
        throw std::invalid_argument("Can not set buffers of \"" + Buffer::GetBufferTypeName(vertex_buffers.GetType()) +
                                    "\" type where \"Vertex\" buffers are required.");
    }

    DrawingState&  drawing_state = GetDrawingState();
    if (drawing_state.vertex_buffer_set_ptr.get() == std::addressof(vertex_buffers))
        return;

    Ptr<ObjectBase> vertex_buffer_set_object_ptr = static_cast<BufferSetBase&>(vertex_buffers).GetBasePtr();
    drawing_state.vertex_buffer_set_ptr = std::static_pointer_cast<BufferSetBase>(vertex_buffer_set_object_ptr);
    drawing_state.changes |= DrawingState::Changes::VertexBuffers;
    RetainResource(std::move(vertex_buffer_set_object_ptr));
}

void RenderCommandListBase::DrawIndexed(Primitive primitive_type, Buffer& index_buffer,
                                        uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                                        uint32_t instance_count, uint32_t /*start_instance*/)
{
    META_FUNCTION_TASK();
    VerifyEncodingState();

    if (m_is_validation_enabled)
    {
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
    }

    UpdateDrawingState(primitive_type, &index_buffer);
}

void RenderCommandListBase::Draw(Primitive primitive_type, uint32_t vertex_count, uint32_t start_vertex,
                                 uint32_t instance_count, uint32_t)
{
    META_FUNCTION_TASK();
    VerifyEncodingState();

    if (m_is_validation_enabled)
    {
        if (vertex_count == 0)
        {
            throw std::invalid_argument("Can not draw zero vertices.");
        }
        if (instance_count == 0)
        {
            throw std::invalid_argument("Can not draw zero instances.");
        }

        ValidateDrawVertexBuffers(start_vertex, vertex_count);
    }

    UpdateDrawingState(primitive_type);
}

void RenderCommandListBase::ResetCommandState()
{
    META_FUNCTION_TASK();
    CommandListBase::ResetCommandState();

    m_drawing_state.render_pass_attachments_ptr.clear();
    m_drawing_state.render_state_ptr.reset();
    m_drawing_state.vertex_buffer_set_ptr.reset();
    m_drawing_state.index_buffer_ptr.reset();
    m_drawing_state.opt_primitive_type.reset();
    m_drawing_state.p_view_state    = nullptr;
    m_drawing_state.render_state_groups = RenderState::Group::None;
    m_drawing_state.changes = DrawingState::Changes::None;
}

void RenderCommandListBase::UpdateDrawingState(Primitive primitive_type, Buffer* p_index_buffer)
{
    META_FUNCTION_TASK();
    DrawingState& drawing_state = GetDrawingState();

    if (p_index_buffer && (!drawing_state.index_buffer_ptr || drawing_state.index_buffer_ptr.get() != p_index_buffer))
    {
        Ptr<ObjectBase> index_buffer_object_ptr = static_cast<BufferBase&>(*p_index_buffer).GetBasePtr();
        drawing_state.index_buffer_ptr = std::static_pointer_cast<BufferBase>(index_buffer_object_ptr);
        drawing_state.changes |= DrawingState::Changes::IndexBuffer;
        RetainResource(std::move(index_buffer_object_ptr));
    }

    if (!drawing_state.opt_primitive_type || *drawing_state.opt_primitive_type != primitive_type)
    {
        drawing_state.changes |= DrawingState::Changes::PrimitiveType;
        drawing_state.opt_primitive_type = primitive_type;
    }
}

void RenderCommandListBase::ValidateDrawVertexBuffers(uint32_t draw_start_vertex, uint32_t draw_vertex_count)
{
    META_FUNCTION_TASK();
    const Data::Size vertex_buffers_count = m_drawing_state.vertex_buffer_set_ptr->GetCount();
    for (Data::Index vertex_buffer_index = 0u; vertex_buffer_index < vertex_buffers_count; ++vertex_buffer_index)
    {
        Buffer&        vertex_buffer = (*m_drawing_state.vertex_buffer_set_ptr)[vertex_buffer_index];
        const uint32_t vertex_count  = vertex_buffer.GetFormattedItemsCount();
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
    assert(!!m_render_pass_ptr);
    return static_cast<RenderPassBase&>(*m_render_pass_ptr);
}

} // namespace Methane::Graphics
