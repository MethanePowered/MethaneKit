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

FILE: Methane/Graphics/RenderCommandListBase.h
Base implementation of the render command list interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RenderCommandList.h>
#include <Methane/Graphics/RenderPass.h>
#include <Methane/Graphics/RenderState.h>

#include "CommandListBase.h"
#include "BufferBase.h"
#include "RenderPassBase.h"

#include <optional>

namespace Methane::Graphics
{

struct RenderState;

class RenderCommandListBase
    : public RenderCommandList
    , public CommandListBase
{
public:
    using Ptr = std::shared_ptr<RenderCommandList>;

    RenderCommandListBase(CommandQueueBase& command_queue, RenderPassBase& render_pass);

    // RenderCommandList interface
    void Reset(RenderState& render_state, const std::string& debug_group = "") override;
    void SetVertexBuffers(const Buffer::Refs& vertex_buffers) override;
    void DrawIndexed(Primitive primitive_type, Buffer& index_buffer,
                     uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                     uint32_t instance_count, uint32_t start_instance) override;
    void Draw(Primitive primitive_type, uint32_t vertex_count, uint32_t start_vertex,
              uint32_t instance_count, uint32_t start_instance) override;

    RenderPassBase& GetPass();

protected:
    void ValidateDrawVertexBuffers(uint32_t draw_start_vertex, uint32_t draw_vertex_count = 0);

    struct DrawStateBase
    {
        struct Flags
        {
            bool primitive_type_changed = false;
            bool index_buffer_changed   = false;
            bool vertex_buffers_changed = false;
        };

        std::optional<Primitive> opt_primitive_type;
        BufferBase::Ptr          sp_index_buffer;
        BufferBase::Ptrs         sp_vertex_buffers;

        Flags                    flags;

        void Reset();
    };

    const RenderPass::Ptr m_sp_pass;
    DrawStateBase         m_draw_state;
};

} // namespace Methane::Graphics
