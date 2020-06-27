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
                None          = 0u,
                PrimitiveType = 1u << 0u,
                IndexBuffer   = 1u << 1u,
                VertexBuffers = 1u << 2u,
                All           = ~0u,
            };

            Changes() = delete;
        };

        // NOTE: justification why raw pointers are used is provided in base class notice, see CommandState for more details

        std::optional<Primitive> opt_primitive_type;
        RawPtr<BufferBase>       p_index_buffer      = nullptr;
        RawPtrs<BufferBase>      vertex_buffers;
        RenderStateBase*         p_render_state      = nullptr;
        RenderState::Group::Mask render_state_groups = RenderState::Group::None;
        Changes::Mask            changes             = Changes::None;
    };

    RenderCommandListBase(CommandQueueBase& command_queue, RenderPassBase& render_pass);
    RenderCommandListBase(ParallelRenderCommandListBase& parallel_render_command_list);
    
    using CommandListBase::Reset;

    // RenderCommandList interface
    void Reset(const Ptr<RenderState>& sp_render_state, DebugGroup* p_debug_group = nullptr) override;
    void SetState(RenderState& render_state, RenderState::Group::Mask state_groups = RenderState::Group::All) override;
    void SetVertexBuffers(const BufferSet& vertex_buffers) override;
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
    Ptr<ParallelRenderCommandListBase> GetParallelRenderCommandList() { return m_wp_parallel_render_command_list.lock(); }

    void ValidateDrawVertexBuffers(uint32_t draw_start_vertex, uint32_t draw_vertex_count = 0);

private:
    const bool                             m_is_parallel;
    const Ptr<RenderPass>                  m_sp_pass;
    WeakPtr<ParallelRenderCommandListBase> m_wp_parallel_render_command_list;
    DrawingState                           m_drawing_state;
};

} // namespace Methane::Graphics
