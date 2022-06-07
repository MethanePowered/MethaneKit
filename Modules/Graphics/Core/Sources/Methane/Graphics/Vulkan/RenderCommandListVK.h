/******************************************************************************

Copyright 2019-2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/RenderCommandListVK.h
Vulkan implementation of the render command list interface.

******************************************************************************/

#pragma once

#include "CommandListVK.hpp"

#include <Methane/Graphics/RenderCommandListBase.h>
#include <Methane/Data/Receiver.hpp>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics
{

class CommandQueueVK;
class BufferVK;
class RenderPassVK;
class ParallelRenderCommandListVK;

class RenderCommandListVK final
    : public CommandListVK<RenderCommandListBase, vk::PipelineBindPoint::eGraphics, 2U, ICommandListVK::CommandBufferType::SecondaryRenderPass>
    , private Data::Receiver<IRenderPassCallback>
{
public:
    explicit RenderCommandListVK(CommandQueueVK& command_queue);
    RenderCommandListVK(CommandQueueVK& command_queue, RenderPassVK& render_pass);
    explicit RenderCommandListVK(ParallelRenderCommandListVK& parallel_render_command_list);

    // CommandList interface
    void Commit() override;

    // RenderCommandList interface
    void Reset(DebugGroup* p_debug_group = nullptr) override;
    void ResetWithState(RenderState& render_state, DebugGroup* p_debug_group = nullptr) override;
    bool SetVertexBuffers(BufferSet& vertex_buffers, bool set_resource_barriers) override;
    bool SetIndexBuffer(Buffer& index_buffer, bool set_resource_barriers) override;
    void DrawIndexed(Primitive primitive, uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                     uint32_t instance_count, uint32_t start_instance) override;
    void Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
              uint32_t instance_count, uint32_t start_instance) override;

private:
    // IRenderPassCallback
    void OnRenderPassUpdated(const RenderPass& render_pass) override;

    void UpdatePrimitiveTopology(Primitive primitive);

    RenderPassVK& GetPassVK();

    const bool m_is_parallel = false;
};

} // namespace Methane::Graphics
