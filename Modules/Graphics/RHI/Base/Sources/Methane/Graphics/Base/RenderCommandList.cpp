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

FILE: Methane/Graphics/Base/RenderCommandList.cpp
Base implementation of the render command list interface.

******************************************************************************/

#include <Methane/Graphics/Base/RenderCommandList.h>
#include <Methane/Graphics/Base/ParallelRenderCommandList.h>
#include <Methane/Graphics/Base/CommandQueue.h>
#include <Methane/Graphics/Base/RenderPass.h>
#include <Methane/Graphics/Base/RenderState.h>
#include <Methane/Graphics/Base/ViewState.h>
#include <Methane/Graphics/Base/Buffer.h>
#include <Methane/Graphics/Base/BufferSet.h>
#include <Methane/Graphics/Base/Program.h>
#include <Methane/Graphics/Base/Texture.h>

#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>

namespace Methane::Graphics::Base
{

RenderCommandList::RenderCommandList(CommandQueue& command_queue)
    : CommandList(command_queue, Type::Render)
{ }

RenderCommandList::RenderCommandList(CommandQueue& command_queue, RenderPass& pass)
    : CommandList(command_queue, Type::Render)
    , m_render_pass_ptr(pass.GetPtr<RenderPass>())
{ }

RenderCommandList::RenderCommandList(ParallelRenderCommandList& parallel_render_command_list)
    : CommandList(static_cast<CommandQueue&>(parallel_render_command_list.GetCommandQueue()), Type::Render)
    , m_is_parallel(true)
    , m_render_pass_ptr(parallel_render_command_list.GetRenderPass().GetPtr<RenderPass>())
{ }

Rhi::IRenderPass& RenderCommandList::GetRenderPass() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_render_pass_ptr);
    return *m_render_pass_ptr;
}

void RenderCommandList::Reset(IDebugGroup* debug_group_ptr)
{
    META_FUNCTION_TASK();
    CommandList::Reset(debug_group_ptr);
    if (m_render_pass_ptr)
    {
        META_LOG("{}", static_cast<std::string>(m_render_pass_ptr->GetPattern().GetSettings()));
        m_drawing_state.render_pass_attachment_ptrs = m_render_pass_ptr->GetNonFrameBufferAttachmentTextures();
    }
}

void RenderCommandList::ResetWithState(Rhi::IRenderState& render_state, IDebugGroup* debug_group_ptr)
{
    META_FUNCTION_TASK();
    RenderCommandList::Reset(debug_group_ptr);
    SetRenderState(render_state);
}

void RenderCommandList::ResetWithStateOnce(Rhi::IRenderState& render_state, IDebugGroup* debug_group_ptr)
{
    META_FUNCTION_TASK();
    if (GetState() == State::Encoding && GetDrawingState().render_state_ptr.get() == std::addressof(render_state))
    {
        META_LOG("{} Command list '{}' was already RESET with the same render state '{}'", magic_enum::enum_name(GetType()), GetName(), render_state.GetName());
        return;
    }
    ResetWithState(render_state, debug_group_ptr);
}

void RenderCommandList::SetRenderState(Rhi::IRenderState& render_state, Rhi::RenderStateGroupMask state_groups)
{
    META_FUNCTION_TASK();
    META_LOG("{} Command list '{}' SET RENDER STATE '{}':\n{}", magic_enum::enum_name(GetType()), GetName(), render_state.GetName(), static_cast<std::string>(render_state.GetSettings()));

    VerifyEncodingState();

    const bool render_state_changed = m_drawing_state.render_state_ptr.get() != std::addressof(render_state);
    Rhi::RenderStateGroupMask changed_states;
    if (!m_drawing_state.render_state_ptr)
        changed_states = Rhi::RenderStateGroupMask(~0U);

    if (m_drawing_state.render_state_ptr && render_state_changed)
    {
        changed_states = Rhi::RenderStateSettings::Compare(render_state.GetSettings(),
                                                           m_drawing_state.render_state_ptr->GetSettings(),
                                                           m_drawing_state.render_state_groups);
    }
    changed_states |= ~m_drawing_state.render_state_groups;

    auto& render_state_base = static_cast<RenderState&>(render_state);
    if (!render_state_base.IsDeferred())
    {
        render_state_base.Apply(*this, changed_states & state_groups);
    }

    Ptr<Object> render_state_object_ptr = render_state_base.GetBasePtr();
    m_drawing_state.render_state_ptr = std::static_pointer_cast<RenderState>(render_state_object_ptr);
    m_drawing_state.render_state_groups |= state_groups;

    if (render_state_changed && !render_state_base.IsDeferred())
    {
        RetainResource(render_state_object_ptr);
    }
}

void RenderCommandList::SetViewState(Rhi::IViewState& view_state)
{
    META_FUNCTION_TASK();
    VerifyEncodingState();

    DrawingState& drawing_state = GetDrawingState();
    if (drawing_state.view_state_ptr && drawing_state.view_state_ptr->GetSettings() == view_state.GetSettings())
    {
        META_LOG("{} Command list '{}' view state is already set up", magic_enum::enum_name(GetType()), GetName());
        return;
    }

    META_LOG("{} Command list '{}' SET VIEW STATE:\n{}",
             magic_enum::enum_name(GetType()), GetName(),
             drawing_state.view_state_ptr ? static_cast<std::string>(drawing_state.view_state_ptr->GetSettings()) : std::string());
    drawing_state.view_state_ptr = static_cast<ViewState*>(&view_state);
    drawing_state.view_state_ptr->Apply(*this);
    drawing_state.changes |= DrawingState::Change::ViewState;
}

bool RenderCommandList::SetVertexBuffers(Rhi::IBufferSet& vertex_buffers, bool set_resource_barriers)
{
    META_FUNCTION_TASK();
    META_UNUSED(set_resource_barriers);

    VerifyEncodingState();

    if (m_is_validation_enabled)
    {
        META_CHECK_ARG_NAME_DESCR("vertex_buffers", vertex_buffers.GetType() == Rhi::BufferType::Vertex,
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

    Ptr<Object> vertex_buffer_set_object_ptr = static_cast<BufferSet&>(vertex_buffers).GetBasePtr();
    drawing_state.vertex_buffer_set_ptr = std::static_pointer_cast<BufferSet>(vertex_buffer_set_object_ptr);
    RetainResource(vertex_buffer_set_object_ptr);
    return true;
}

bool RenderCommandList::SetIndexBuffer(Rhi::IBuffer& index_buffer, bool set_resource_barriers)
{
    META_FUNCTION_TASK();
    META_UNUSED(set_resource_barriers);

    VerifyEncodingState();

    if (m_is_validation_enabled)
    {
        META_CHECK_ARG_NAME_DESCR("index_buffer", index_buffer.GetSettings().type == Rhi::BufferType::Index,
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

    Ptr<Object> index_buffer_object_ptr = static_cast<Buffer&>(index_buffer).GetBasePtr();
    drawing_state.index_buffer_ptr = std::static_pointer_cast<Buffer>(index_buffer_object_ptr);
    RetainResource(index_buffer_object_ptr);
    return true;
}

void RenderCommandList::DrawIndexed(Primitive primitive_type, uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
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

void RenderCommandList::Draw(Primitive primitive_type, uint32_t vertex_count, uint32_t start_vertex,
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

void RenderCommandList::ResetCommandState()
{
    META_FUNCTION_TASK();
    META_LOG("{} Command list '{}' reset command state", magic_enum::enum_name(GetType()), GetName());

    CommandList::ResetCommandState();

    m_drawing_state.render_pass_attachment_ptrs.clear();
    m_drawing_state.render_state_ptr.reset();
    m_drawing_state.vertex_buffer_set_ptr.reset();
    m_drawing_state.index_buffer_ptr.reset();
    m_drawing_state.primitive_type_opt.reset();
    m_drawing_state.view_state_ptr = nullptr;
    m_drawing_state.render_state_groups = {};
    m_drawing_state.changes = DrawingState::ChangeMask{};
}

void RenderCommandList::UpdateDrawingState(Primitive primitive_type)
{
    META_FUNCTION_TASK();
    DrawingState& drawing_state = GetDrawingState();
    if (!drawing_state.primitive_type_opt || *drawing_state.primitive_type_opt == primitive_type)
    {
        drawing_state.changes |= DrawingState::Change::PrimitiveType;
        drawing_state.primitive_type_opt = primitive_type;
    }

    if (m_drawing_state.render_state_ptr &&
        m_drawing_state.render_state_ptr->IsDeferred() &&
        (static_cast<bool>(m_drawing_state.render_state_groups) ||
         drawing_state.changes.HasAnyBit(DrawingState::Change::PrimitiveType) ||
         drawing_state.changes.HasAnyBit(DrawingState::Change::ViewState)))
    {
        // Apply render state in deferred mode right before the Draw call,
        // only in case when any render state groups or view state or primitive type has changed
        m_drawing_state.render_state_ptr->Apply(*this, m_drawing_state.render_state_groups);
        RetainResource(m_drawing_state.render_state_ptr);

        m_drawing_state.render_state_groups = {};
        drawing_state.changes.SetBitOff(DrawingState::Change::PrimitiveType);
        drawing_state.changes.SetBitOff(DrawingState::Change::ViewState);
    }
}

void RenderCommandList::ValidateDrawVertexBuffers(uint32_t draw_start_vertex, uint32_t draw_vertex_count) const
{
    META_FUNCTION_TASK();
    META_UNUSED(draw_vertex_count);
    if (!m_drawing_state.vertex_buffer_set_ptr)
        return;

    const Data::Size vertex_buffers_count = m_drawing_state.vertex_buffer_set_ptr->GetCount();
    for (Data::Index vertex_buffer_index = 0U; vertex_buffer_index < vertex_buffers_count; ++vertex_buffer_index)
    {
        const Rhi::IBuffer&  vertex_buffer = (*m_drawing_state.vertex_buffer_set_ptr)[vertex_buffer_index];
        const uint32_t vertex_count  = vertex_buffer.GetFormattedItemsCount();
        META_UNUSED(vertex_count);
        META_CHECK_ARG_LESS_DESCR(draw_start_vertex, vertex_count - draw_vertex_count + 1U,
                                  "can not draw starting from vertex {}{} which is out of bounds for vertex buffer '{}' with vertex count {}",
                                  draw_start_vertex, draw_vertex_count ? fmt::format(" with {} vertex count", draw_vertex_count) : "",
                                  vertex_buffer.GetName(), vertex_count);
    }
}

RenderPass& RenderCommandList::GetPass()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_render_pass_ptr);
    return *m_render_pass_ptr;
}

} // namespace Methane::Graphics::Base
