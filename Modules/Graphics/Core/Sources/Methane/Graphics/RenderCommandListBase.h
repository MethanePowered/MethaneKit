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
        enum class Changes : uint32_t
        {
            None          = 0U,
            PrimitiveType = 1U << 0U,
            All           = ~0U
        };

        Ptrs<TextureBase>        render_pass_attachments_ptr;
        Ptr<RenderStateBase>     render_state_ptr;
        Ptr<BufferSetBase>       vertex_buffer_set_ptr;
        Ptr<BufferBase>          index_buffer_ptr;
        std::optional<Primitive> opt_primitive_type;
        ViewStateBase*           p_view_state        = nullptr;
        RenderState::Groups      render_state_groups = RenderState::Groups::None;
        Changes                  changes             = Changes::None;
    };

    static Ptr<RenderCommandList> CreateForSynchronization(CommandQueue& cmd_queue);

    explicit RenderCommandListBase(CommandQueueBase& command_queue);
    RenderCommandListBase(CommandQueueBase& command_queue, RenderPassBase& render_pass);
    explicit RenderCommandListBase(ParallelRenderCommandListBase& parallel_render_command_list);
    
    using CommandListBase::Reset;

    // RenderCommandList interface
    bool IsValidationEnabled() const noexcept override                      { return m_is_validation_enabled; }
    void SetValidationEnabled(bool is_validation_enabled) override          { m_is_validation_enabled = is_validation_enabled; }
    RenderPass& GetRenderPass() const noexcept override                     { return *m_render_pass_ptr; }
    void Reset(DebugGroup* p_debug_group = nullptr) override;
    void ResetWithState(RenderState& render_state, DebugGroup* p_debug_group = nullptr) override;
    void ResetWithStateOnce(RenderState& render_state, DebugGroup* p_debug_group = nullptr) final;
    void SetRenderState(RenderState& render_state, RenderState::Groups state_groups = RenderState::Groups::All) override;
    void SetViewState(ViewState& view_state) override;
    bool SetVertexBuffers(BufferSet& vertex_buffers, bool set_resource_barriers) override;
    bool SetIndexBuffer(Buffer& index_buffer, bool set_resource_barriers) override;
    void DrawIndexed(Primitive primitive_type, uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                     uint32_t instance_count, uint32_t start_instance) override;
    void Draw(Primitive primitive_type, uint32_t vertex_count, uint32_t start_vertex,
              uint32_t instance_count, uint32_t start_instance) override;

    bool            HasPass() const noexcept    { return !!m_render_pass_ptr; }
    RenderPassBase* GetPassPtr() const noexcept { return m_render_pass_ptr.get(); }
    RenderPassBase& GetPass();

protected:
    // CommandListBase overrides
    void ResetCommandState() override;

    DrawingState&                      GetDrawingState()                    { return m_drawing_state; }
    const DrawingState&                GetDrawingState() const              { return m_drawing_state; }
    bool                               IsParallel() const                   { return m_is_parallel; }
    Ptr<ParallelRenderCommandListBase> GetParallelRenderCommandList() const { return m_parallel_render_command_list_wptr.lock(); }

    inline void UpdateDrawingState(Primitive primitive_type);
    inline void ValidateDrawVertexBuffers(uint32_t draw_start_vertex, uint32_t draw_vertex_count = 0) const;

private:
    const bool                             m_is_parallel = false;
    const Ptr<RenderPassBase>              m_render_pass_ptr;
    WeakPtr<ParallelRenderCommandListBase> m_parallel_render_command_list_wptr;
    DrawingState                           m_drawing_state;
    bool                                   m_is_validation_enabled = true;
};

} // namespace Methane::Graphics
