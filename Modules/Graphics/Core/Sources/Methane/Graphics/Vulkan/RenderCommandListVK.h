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

FILE: Methane/Graphics/Vulkan/RenderCommandListVK.h
Vulkan implementation of the render command list interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RenderCommandListBase.h>

#include <functional>

namespace Methane::Graphics
{

class CommandQueueVK;
class BufferVK;
class RenderPassVK;

class RenderCommandListVK final : public RenderCommandListBase
{
public:
    RenderCommandListVK(CommandQueueBase& command_queue, RenderPassBase& render_pass);
    RenderCommandListVK(ParallelRenderCommandListBase& parallel_render_command_list);

    // CommandList interface
    void PushDebugGroup(const std::string& name) override;
    void PopDebugGroup() override;
    void Commit() override;

    // CommandListBase interface
    void SetResourceBarriers(const ResourceBase::Barriers&) override { }
    void Execute(uint32_t frame_index) override;

    // RenderCommandList interface
    void Reset(const Ptr<RenderState>& sp_render_state, const std::string& debug_group = "") override;
    void SetVertexBuffers(const Refs<Buffer>& vertex_buffers) override;
    void DrawIndexed(Primitive primitive, Buffer& index_buffer,
                     uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                     uint32_t instance_count, uint32_t start_instance) override;
    void Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
              uint32_t instance_count, uint32_t start_instance) override;

    // Object interface
    void SetName(const std::string& label) override;

protected:
    CommandQueueVK& GetCommandQueueVK() noexcept;
    RenderPassVK&   GetPassVK();
};

} // namespace Methane::Graphics
