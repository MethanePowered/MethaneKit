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

FILE: Methane/Graphics/RenderCommandListBase.h
Base implementation of the render command list interface.

******************************************************************************/

#pragma once

#include "CommandListBase.h"
#include "BufferBase.h"
#include "RenderPassBase.h"
#include "RenderStateBase.h"

#include <Methane/Graphics/RenderCommandList.h>

#include <optional>

namespace Methane::Graphics
{

struct RenderState;
class ParallelRenderCommandListBase;

class RenderCommandListBase
    : public RenderCommandList
    , public CommandListBase
{
public:
    struct DrawingState final
    {
        struct Changes
        {
            using Mask = uint32_t;
            enum Value : Mask
            {
                None          = 0U,
                PrimitiveType = 1U << 0U,
                IndexBuffer   = 1U << 1U,
                VertexBuffers = 1U << 2U,
                All           = ~0U,
            };

            Changes() = delete;
        };

        Ptrs<TextureBase>        render_pass_attachments_ptr;
        Ptr<RenderStateBase>     render_state_ptr;
        Ptr<BufferSetBase>       vertex_buffer_set_ptr;
        Ptr<BufferBase>          index_buffer_ptr;
        std::optional<Primitive> opt_primitive_type;
        ViewStateBase*           p_view_state        = nullptr;
        RenderState::Group::Mask render_state_groups = RenderState::Group::None;
        Changes::Mask            changes             = Changes::None;
    };

    RenderCommandListBase(CommandQueueBase& command_queue, RenderPassBase& render_pass);
    RenderCommandListBase(ParallelRenderCommandListBase& parallel_render_command_list);
    
    using CommandListBase::Reset;

    // RenderCommandList interface
    bool IsValidationEnabled() const noexcept override                      { return m_is_validation_enabled; }
    void SetValidationEnabled(bool is_validation_enabled) noexcept override { m_is_validation_enabled = is_validation_enabled; }
    RenderPass& GetRenderPass() const noexcept override                     { return *m_render_pass_ptr; }
    void Reset(const Ptr<RenderState>& render_state_ptr, DebugGroup* p_debug_group = nullptr) override;
    void SetRenderState(RenderState& render_state, RenderState::Group::Mask state_groups = RenderState::Group::All) override;
    void SetViewState(ViewState& view_state) override;
    void SetVertexBuffers(BufferSet& vertex_buffers) override;
    void DrawIndexed(Primitive primitive_type, Buffer& index_buffer,
                     uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                     uint32_t instance_count, uint32_t start_instance) override;
    void Draw(Primitive primitive_type, uint32_t vertex_count, uint32_t start_vertex,
              uint32_t instance_count, uint32_t start_instance) override;

    RenderPassBase& GetPass();

protected:
    // CommandListBase overrides
    void ResetCommandState() override;

    DrawingState&                      GetDrawingState()              { return m_drawing_state; }
    const DrawingState&                GetDrawingState() const        { return m_drawing_state; }
    bool                               IsParallel() const             { return m_is_parallel; }
    Ptr<ParallelRenderCommandListBase> GetParallelRenderCommandList() { return m_parallel_render_command_list_wptr.lock(); }

    inline void UpdateDrawingState(Primitive primitive_type, Buffer* p_index_buffer = nullptr);
    inline void ValidateDrawVertexBuffers(uint32_t draw_start_vertex, uint32_t draw_vertex_count = 0);

private:
    const bool                             m_is_parallel;
    const Ptr<RenderPassBase>              m_render_pass_ptr;
    WeakPtr<ParallelRenderCommandListBase> m_parallel_render_command_list_wptr;
    DrawingState                           m_drawing_state;
    bool                                   m_is_validation_enabled = true;
};

} // namespace Methane::Graphics
