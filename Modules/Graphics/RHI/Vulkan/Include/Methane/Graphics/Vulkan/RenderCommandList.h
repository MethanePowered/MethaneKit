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

FILE: Methane/Graphics/Vulkan/RenderCommandList.h
Vulkan implementation of the render command list interface.

******************************************************************************/

#pragma once

#include "CommandList.hpp"

#include <Methane/Graphics/Base/RenderCommandList.h>
#include <Methane/Graphics/RHI/IRenderPass.h>
#include <Methane/Data/Receiver.hpp>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics::Vulkan
{

class CommandQueue;
class Buffer;
class RenderPass;
class ParallelRenderCommandList;

class RenderCommandList final // NOSONAR - inheritance hierarchy is greater than 5
    : public CommandList<Base::RenderCommandList, vk::PipelineBindPoint::eGraphics, 2U, CommandBufferType::SecondaryRenderPass>
    , private Data::Receiver<Rhi::IRenderPassCallback>
{
public:
    explicit RenderCommandList(CommandQueue& command_queue);
    RenderCommandList(CommandQueue& command_queue, RenderPass& render_pass);
    explicit RenderCommandList(ParallelRenderCommandList& parallel_render_command_list, bool is_beginning_cmd_list);

    // ICommandList interface
    void Commit() override;

    // IRenderCommandList interface
    void Reset(IDebugGroup* debug_group_ptr = nullptr) override;
    void ResetWithState(Rhi::IRenderState& render_state, IDebugGroup* debug_group_ptr = nullptr) override;
    bool SetVertexBuffers(Rhi::IBufferSet& vertex_buffers, bool set_resource_barriers) override;
    bool SetIndexBuffer(Rhi::IBuffer& index_buffer, bool set_resource_barriers) override;
    void DrawIndexed(Primitive primitive, uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                     uint32_t instance_count, uint32_t start_instance) override;
    void Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
              uint32_t instance_count, uint32_t start_instance) override;

    bool IsDynamicStateSupported() const noexcept { return m_is_dynamic_state_supported; }

    // IRenderPassCallback
    void OnRenderPassUpdated(const Rhi::IRenderPass& render_pass) override;

private:
    void UpdatePrimitiveTopology(Primitive primitive);

    RenderPass& GetVulkanPass();

    const bool m_is_dynamic_state_supported;
};

} // namespace Methane::Graphics::Vulkan
