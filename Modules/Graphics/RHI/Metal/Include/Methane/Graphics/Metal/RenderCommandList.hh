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

FILE: Methane/Graphics/Metal/RenderCommandList.hh
Metal implementation of the render command list interface.

******************************************************************************/

#pragma once

#include "CommandList.hpp"

#include <Methane/Graphics/Base/RenderCommandList.h>

#import <Metal/Metal.h>

namespace Methane::Graphics::Metal
{

class CommandQueue;
class Buffer;
class RenderPass;
class ParallelRenderCommandList;

class RenderCommandList final
    : public CommandList<id<MTLRenderCommandEncoder>, Base::RenderCommandList>
{
public:
    RenderCommandList(CommandQueue& command_queue, Base::RenderPass& render_pass);
    explicit RenderCommandList(ParallelRenderCommandList& parallel_render_command_list);

    // IRenderCommandList interface
    void Reset(IDebugGroup* debug_group_ptr = nullptr) override;
    void ResetWithState(Rhi::IRenderState& render_state, IDebugGroup* debug_group_ptr = nullptr) override;
    bool SetVertexBuffers(Rhi::IBufferSet& vertex_buffers, bool set_resource_barriers) override;
    void DrawIndexed(Primitive primitive, uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                     uint32_t instance_count, uint32_t start_instance) override;
    void Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
              uint32_t instance_count, uint32_t start_instance) override;

private:
    RenderPass& GetMetalRenderPass();
    void ResetCommandEncoder();

    const ParallelRenderCommandList* m_parallel_render_command_list_ptr = nullptr;
    const bool m_device_supports_gpu_family_apple_3;
};

} // namespace Methane::Graphics::Metal
