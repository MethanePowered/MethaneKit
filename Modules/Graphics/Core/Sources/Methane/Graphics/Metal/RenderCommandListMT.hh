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

FILE: Methane/Graphics/Metal/RenderCommandListMT.hh
Metal implementation of the render command list interface.

******************************************************************************/

#pragma once

#include "CommandListMT.hpp"

#include <Methane/Graphics/RenderCommandListBase.h>

#import <Metal/Metal.h>

namespace Methane::Graphics
{

class CommandQueueMT;
class BufferMT;
class RenderPassMT;

class RenderCommandListMT final
    : public CommandListMT<id<MTLRenderCommandEncoder>, RenderCommandListBase>
{
public:
    RenderCommandListMT(CommandQueueBase& command_queue, RenderPassBase& render_pass);
    explicit RenderCommandListMT(ParallelRenderCommandListBase& parallel_render_command_list);

    // RenderCommandList interface
    void Reset(DebugGroup* p_debug_group = nullptr) override;
    void ResetWithState(RenderState& render_state, DebugGroup* p_debug_group = nullptr) override;
    void SetVertexBuffers(BufferSet& vertex_buffers, bool set_resource_barriers) override;
    void DrawIndexed(Primitive primitive, Buffer& index_buffer,
                     uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                     uint32_t instance_count, uint32_t start_instance) override;
    void Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
              uint32_t instance_count, uint32_t start_instance) override;

private:
    RenderPassMT& GetRenderPassMT();
    void ResetCommandEncoder();
};

} // namespace Methane::Graphics
