/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Null/RenderCommandList.h
Null implementation of the render command list interface.

******************************************************************************/

#pragma once

#include "CommandList.hpp"
#include "RenderPass.h"

#include <Methane/Graphics/Base/RenderCommandList.h>

namespace Methane::Graphics::Null
{

class CommandQueue;
class Buffer;
class ParallelRenderCommandList;

class RenderCommandList final // NOSONAR - inheritance hierarchy is greater than 5
    : public CommandList<Base::RenderCommandList>
{
public:
    explicit RenderCommandList(CommandQueue& command_queue);
    RenderCommandList(CommandQueue& command_queue, RenderPass& render_pass);
    explicit RenderCommandList(ParallelRenderCommandList& parallel_render_command_list);

    // IRenderCommandList interface
    void Reset(IDebugGroup* debug_group_ptr = nullptr) override;
    void ResetWithState(Rhi::IRenderState& render_state, IDebugGroup* debug_group_ptr = nullptr) override;
    bool SetVertexBuffers(Rhi::IBufferSet& vertex_buffers, bool set_resource_barriers) override;
    bool SetIndexBuffer(Rhi::IBuffer& index_buffer, bool set_resource_barriers) override;
    void DrawIndexed(Primitive primitive, uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                     uint32_t instance_count, uint32_t start_instance) override;
    void Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
              uint32_t instance_count, uint32_t start_instance) override;

    using Base::RenderCommandList::GetDrawingState;
    using Base::CommandList::GetCommandState;
};

} // namespace Methane::Graphics::Null
