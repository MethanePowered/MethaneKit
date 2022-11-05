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

#include <Methane/Graphics/Base/RenderCommandList.h>
#include <Methane/Data/Receiver.hpp>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics
{

class CommandQueueVK;
class BufferVK;
class RenderPassVK;
class ParallelRenderCommandListVK;

class RenderCommandListVK final // NOSONAR - inheritance hierarchy is greater than 5
    : public CommandListVK<Base::RenderCommandList, vk::PipelineBindPoint::eGraphics, 2U, ICommandListVK::CommandBufferType::SecondaryRenderPass>
    , private Data::Receiver<IRenderPassCallback>
{
public:
    explicit RenderCommandListVK(CommandQueueVK& command_queue);
    RenderCommandListVK(CommandQueueVK& command_queue, RenderPassVK& render_pass);
    explicit RenderCommandListVK(ParallelRenderCommandListVK& parallel_render_command_list, bool is_beginning_cmd_list);

    // ICommandList interface
    void Commit() override;

    // IRenderCommandList interface
    void Reset(IDebugGroup* p_debug_group = nullptr) override;
    void ResetWithState(IRenderState& render_state, IDebugGroup* p_debug_group = nullptr) override;
    bool SetVertexBuffers(IBufferSet& vertex_buffers, bool set_resource_barriers) override;
    bool SetIndexBuffer(IBuffer& index_buffer, bool set_resource_barriers) override;
    void DrawIndexed(Primitive primitive, uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                     uint32_t instance_count, uint32_t start_instance) override;
    void Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
              uint32_t instance_count, uint32_t start_instance) override;

private:
    // IRenderPassCallback
    void OnRenderPassUpdated(const IRenderPass& render_pass) override;

    void UpdatePrimitiveTopology(Primitive primitive);

    RenderPassVK& GetPassVK();
};

} // namespace Methane::Graphics
