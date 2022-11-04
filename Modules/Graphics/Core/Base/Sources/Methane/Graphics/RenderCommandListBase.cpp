/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

#include <magic_enum.hpp>

namespace Methane::Graphics
{

RenderCommandListBase::RenderCommandListBase(CommandQueueBase& command_queue)
    : CommandListBase(command_queue, Type::Render)
{
    META_FUNCTION_TASK();
}

RenderCommandListBase::RenderCommandListBase(CommandQueueBase& command_queue, RenderPassBase& pass)
    : CommandListBase(command_queue, Type::Render)
    , m_render_pass_ptr(pass.GetPtr<RenderPassBase>())
{
    META_FUNCTION_TASK();
}

RenderCommandListBase::RenderCommandListBase(ParallelRenderCommandListBase& parallel_render_command_list)
    : CommandListBase(static_cast<CommandQueueBase&>(parallel_render_command_list.GetCommandQueue()), Type::Render)
    , m_is_parallel(true)
    , m_render_pass_ptr(parallel_render_command_list.GetPass().GetPtr<RenderPassBase>())
{
    META_FUNCTION_TASK();
}

IRenderPass& RenderCommandListBase::GetRenderPass() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_render_pass_ptr);
    return *m_render_pass_ptr;
}

void RenderCommandListBase::Reset(IDebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    CommandListBase::Reset(p_debug_group);
    if (m_render_pass_ptr)
    {
        META_LOG("{}", static_cast<std::string>(m_render_pass_ptr->GetPattern().GetSettings()));
        m_drawing_state.render_pass_attachments_ptr = m_render_pass_ptr->GetNonFrameBufferAttachmentTextures();
    }
}

void RenderCommandListBase::ResetWithState(IRenderState& render_state, IDebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    RenderCommandListBase::Reset(p_debug_group);
    SetRenderState(render_state);
}

void RenderCommandListBase::ResetWithStateOnce(IRenderState& render_state, IDebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    if (GetState() == State::Encoding && GetDrawingState().render_state_ptr.get() == std::addressof(render_state))
    {
        META_LOG("{} Command list '{}' was already RESET with the same render state '{}'", magic_enum::enum_name(GetType()), GetName(), render_state.GetName());
        return;
    }
    ResetWithState(render_state, p_debug_group);
}

void RenderCommandListBase::SetRenderState(IRenderState& render_state, RenderStateGroups state_groups)
{
    META_FUNCTION_TASK();
    META_LOG("{} Command list '{}' SET RENDER STATE '{}':\n{}", magic_enum::enum_name(GetType()), GetName(), render_state.GetName(), static_cast<std::string>(render_state.GetSettings()));

    using namespace magic_enum::bitwise_operators;

    VerifyEncodingState();

    const bool           render_state_changed = m_drawing_state.render_state_ptr.get() != std::addressof(render_state);
    IRenderState::Groups changed_states       = m_drawing_state.render_state_ptr ? IRenderState::Groups::None : IRenderState::Groups::All;
    if (m_drawing_state.render_state_ptr && render_state_changed)
    {
        changed_states = IRenderState::Settings::Compare(render_state.GetSettings(),
                                                         m_drawing_state.render_state_ptr->GetSettings(),
                                                         m_drawing_state.render_state_groups);
    }
    changed_states |= ~m_drawing_state.render_state_groups;

    auto& render_state_base = static_cast<RenderStateBase&>(render_state);
    render_state_base.Apply(*this, changed_states & state_groups);

    Ptr<ObjectBase> render_state_object_ptr = render_state_base.GetBasePtr();
    m_drawing_state.render_state_ptr = std::static_pointer_cast<RenderStateBase>(render_state_object_ptr);
    m_drawing_state.render_state_groups |= state_groups;

    if (render_state_changed)
    {
        RetainResource(render_state_object_ptr);
    }
}

void RenderCommandListBase::SetViewState(IViewState& view_state)
{
    META_FUNCTION_TASK();
    VerifyEncodingState();

    DrawingState& drawing_state = GetDrawingState();
    const ViewStateBase* p_prev_view_state = drawing_state.view_state_ptr;
    drawing_state.view_state_ptr = static_cast<ViewStateBase*>(&view_state);

    if (p_prev_view_state && p_prev_view_state->GetSettings() == view_state.GetSettings())
    {
        META_LOG("{} Command list '{}' view state is already set up", magic_enum::enum_name(GetType()), GetName());
        return;
    }

    META_LOG("{} Command list '{}' SET VIEW STATE:\n{}", magic_enum::enum_name(GetType()), GetName(), static_cast<std::string>(drawing_state.view_state_ptr->GetSettings()));
    drawing_state.view_state_ptr->Apply(*this);
}

bool RenderCommandListBase::SetVertexBuffers(IBufferSet& vertex_buffers, bool set_resource_barriers)
{
    META_FUNCTION_TASK();
    META_UNUSED(set_resource_barriers);

    VerifyEncodingState();

    if (m_is_validation_enabled)
    {
        META_CHECK_ARG_NAME_DESCR("vertex_buffers", vertex_buffers.GetType() == IBuffer::Type::Vertex,
                                  "can not set buffers of '{}' type where 'Vertex' buffers are required",
                                  magic_enum::enum_name(vertex_buffers.GetType()));
    }

    DrawingState&  drawing_state = GetDrawingState();
    if (drawing_state.vertex_buffer_set_ptr.get() == std::addressof(vertex_buffers))
    {
        META_LOG("{} Command list '{}' vertex buffers {} are already set up",
                 magic_enum::enum_name(GetType()), GetName(), vertex_buffers.GetNames());
        return false;
    }

    META_LOG("{} Command list '{}' SET VERTEX BUFFERS {}", magic_enum::enum_name(GetType()), GetName(), vertex_buffers.GetNames());

    Ptr<ObjectBase> vertex_buffer_set_object_ptr = static_cast<BufferSetBase&>(vertex_buffers).GetBasePtr();
    drawing_state.vertex_buffer_set_ptr = std::static_pointer_cast<BufferSetBase>(vertex_buffer_set_object_ptr);
    RetainResource(vertex_buffer_set_object_ptr);
    return true;
}

bool RenderCommandListBase::SetIndexBuffer(IBuffer& index_buffer, bool set_resource_barriers)
{
    META_FUNCTION_TASK();
    META_UNUSED(set_resource_barriers);

    VerifyEncodingState();

    if (m_is_validation_enabled)
    {
        META_CHECK_ARG_NAME_DESCR("index_buffer", index_buffer.GetSettings().type == IBuffer::Type::Index,
                                  "can not set with index buffer of type '{}' where 'Index' buffer is required",
                                  magic_enum::enum_name(index_buffer.GetSettings().type));
    }

    DrawingState& drawing_state = GetDrawingState();
    if (drawing_state.index_buffer_ptr.get() == std::addressof(index_buffer))
    {
        META_LOG("{} Command list '{}' index buffer {} is already set up",
                 magic_enum::enum_name(GetType()), GetName(), index_buffer.GetName());
        return false;
    }

    Ptr<ObjectBase> index_buffer_object_ptr = static_cast<BufferBase&>(index_buffer).GetBasePtr();
    drawing_state.index_buffer_ptr = std::static_pointer_cast<BufferBase>(index_buffer_object_ptr);
    RetainResource(index_buffer_object_ptr);
    return true;
}

void RenderCommandListBase::DrawIndexed(Primitive primitive_type, uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                                        uint32_t instance_count, uint32_t start_instance)
{
    META_FUNCTION_TASK();
    VerifyEncodingState();

    if (m_is_validation_enabled)
    {
        const DrawingState& drawing_state = GetDrawingState();
        META_CHECK_ARG_NOT_NULL_DESCR(drawing_state.index_buffer_ptr, "index buffer must be set before indexed draw call");
        META_CHECK_ARG_NOT_NULL_DESCR(drawing_state.vertex_buffer_set_ptr, "vertex buffers must be set before draw call");

        const uint32_t formatted_items_count = drawing_state.index_buffer_ptr->GetFormattedItemsCount();
        META_CHECK_ARG_NOT_ZERO_DESCR(formatted_items_count, "can not draw with index buffer which contains no formatted vertices");
        META_CHECK_ARG_NOT_ZERO_DESCR(index_count, "can not draw zero index/vertex count");
        META_CHECK_ARG_NOT_ZERO_DESCR(instance_count, "can not draw zero instances");
        META_CHECK_ARG_LESS_DESCR(start_index, formatted_items_count - index_count + 1U, "ending index is out of buffer bounds");

        ValidateDrawVertexBuffers(start_vertex);
    }

    META_LOG("{} Command list '{}' DRAW INDEXED with vertex buffers {} and index buffer '{}' using {} primive type, {} indices from {} index and {} vertex with {} instances count from {} instance",
             magic_enum::enum_name(GetType()), GetName(), GetDrawingState().vertex_buffer_set_ptr->GetNames(), GetDrawingState().index_buffer_ptr->GetName(),
             magic_enum::enum_name(primitive_type), index_count, start_index, start_vertex, instance_count, start_instance);
    META_UNUSED(start_instance);

    UpdateDrawingState(primitive_type);
}

void RenderCommandListBase::Draw(Primitive primitive_type, uint32_t vertex_count, uint32_t start_vertex,
                                 uint32_t instance_count, uint32_t start_instance)
{
    META_FUNCTION_TASK();
    VerifyEncodingState();

    if (m_is_validation_enabled)
    {
        const DrawingState& drawing_state = GetDrawingState();
        META_CHECK_ARG_NOT_NULL_DESCR(drawing_state.render_state_ptr, "render state must be set before draw call");
        const size_t input_buffers_count = drawing_state.render_state_ptr->GetSettings().program_ptr->GetSettings().input_buffer_layouts.size();
        META_CHECK_ARG_TRUE_DESCR(!input_buffers_count || drawing_state.vertex_buffer_set_ptr,
                                 "vertex buffers must be set when program has non empty input buffer layouts");
        META_CHECK_ARG_TRUE_DESCR(!drawing_state.vertex_buffer_set_ptr || drawing_state.vertex_buffer_set_ptr->GetCount() == input_buffers_count,
                                  "vertex buffers count must be equal to the program input buffer layouts count");
        META_CHECK_ARG_NOT_ZERO_DESCR(vertex_count, "can not draw zero vertices");
        META_CHECK_ARG_NOT_ZERO_DESCR(instance_count, "can not draw zero instances");

        ValidateDrawVertexBuffers(start_vertex, vertex_count);
    }

    META_LOG("{} Command list '{}' DRAW with vertex buffers {} using {} primitive type, {} vertices from {} vertex with {} instances count from {} instance",
             magic_enum::enum_name(GetType()), GetName(),
             GetDrawingState().vertex_buffer_set_ptr ? GetDrawingState().vertex_buffer_set_ptr->GetNames() : "None",
             magic_enum::enum_name(primitive_type), vertex_count, start_vertex, instance_count, start_instance);
    META_UNUSED(start_instance);

    UpdateDrawingState(primitive_type);
}

void RenderCommandListBase::ResetCommandState()
{
    META_FUNCTION_TASK();
    META_LOG("{} Command list '{}' reset command state", magic_enum::enum_name(GetType()), GetName());

    CommandListBase::ResetCommandState();

    m_drawing_state.render_pass_attachments_ptr.clear();
    m_drawing_state.render_state_ptr.reset();
    m_drawing_state.vertex_buffer_set_ptr.reset();
    m_drawing_state.index_buffer_ptr.reset();
    m_drawing_state.primitive_type_opt.reset();
    m_drawing_state.view_state_ptr = nullptr;
    m_drawing_state.render_state_groups = IRenderState::Groups::None;
    m_drawing_state.changes = DrawingState::Changes::None;
}

void RenderCommandListBase::UpdateDrawingState(Primitive primitive_type)
{
    META_FUNCTION_TASK();
    DrawingState& drawing_state = GetDrawingState();
    if (drawing_state.primitive_type_opt && *drawing_state.primitive_type_opt == primitive_type)
        return;

    using namespace magic_enum::bitwise_operators;
    drawing_state.changes |= DrawingState::Changes::PrimitiveType;
    drawing_state.primitive_type_opt = primitive_type;
}

void RenderCommandListBase::ValidateDrawVertexBuffers(uint32_t draw_start_vertex, uint32_t draw_vertex_count) const
{
    META_FUNCTION_TASK();
    META_UNUSED(draw_vertex_count);
    if (!m_drawing_state.vertex_buffer_set_ptr)
        return;

    const Data::Size vertex_buffers_count = m_drawing_state.vertex_buffer_set_ptr->GetCount();
    for (Data::Index vertex_buffer_index = 0U; vertex_buffer_index < vertex_buffers_count; ++vertex_buffer_index)
    {
        const IBuffer&  vertex_buffer = (*m_drawing_state.vertex_buffer_set_ptr)[vertex_buffer_index];
        const uint32_t vertex_count  = vertex_buffer.GetFormattedItemsCount();
        META_UNUSED(vertex_count);
        META_CHECK_ARG_LESS_DESCR(draw_start_vertex, vertex_count - draw_vertex_count + 1U,
                                  "can not draw starting from vertex {}{} which is out of bounds for vertex buffer '{}' with vertex count {}",
                                  draw_start_vertex, draw_vertex_count ? fmt::format(" with {} vertex count", draw_vertex_count) : "",
                                  vertex_buffer.GetName(), vertex_count);
    }
}

RenderPassBase& RenderCommandListBase::GetPass()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_render_pass_ptr);
    return *m_render_pass_ptr;
}

} // namespace Methane::Graphics
