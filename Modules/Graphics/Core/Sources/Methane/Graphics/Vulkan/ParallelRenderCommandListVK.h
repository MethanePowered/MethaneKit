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

FILE: Methane/Graphics/Vulkan/ParallelRenderCommandListVK.h
Vulkan implementation of the parallel render command list interface.

******************************************************************************/

#pragma once

#include "RenderCommandListVK.h"

#include <Methane/Graphics/ParallelRenderCommandListBase.h>
#include <Methane/Graphics/QueryPoolBase.h>

namespace Methane::Graphics
{

class CommandQueueVK;
class BufferVK;
class RenderPassVK;

class ParallelRenderCommandListVK final : public ParallelRenderCommandListBase
{
public:
    ParallelRenderCommandListVK(CommandQueueVK& command_queue, RenderPassVK& render_pass);

    // ParallelRenderCommandList interface
    void Reset(IDebugGroup* p_debug_group = nullptr) override;
    void ResetWithState(IRenderState& render_state, IDebugGroup* p_debug_group = nullptr) override;
    void SetBeginningResourceBarriers(const IResourceBarriers& resource_barriers) override;
    void SetEndingResourceBarriers(const IResourceBarriers& resource_barriers) override;
    void SetParallelCommandListsCount(uint32_t count) override;

    // ICommandList interface
    void Commit() override;

    // CommandListBase interface
    void Execute(const CompletedCallback& completed_callback = {}) override;
    void Complete() override;

    // IObject interface
    bool SetName(const std::string& label) override;

    const ICommandListVK& GetPrimaryCommandListVK() const noexcept { return m_beginning_command_list; }
    CommandQueueVK& GetCommandQueueVK() noexcept;
    RenderPassVK& GetPassVK() noexcept;

private:
    using SyncCommandListVK = CommandListVK<CommandListBase, vk::PipelineBindPoint::eGraphics>;

    RenderCommandListVK              m_beginning_command_list;
    vk::CommandBufferInheritanceInfo m_vk_ending_inheritance_info;
    SyncCommandListVK                m_ending_command_list;
    std::vector<vk::CommandBuffer>   m_vk_parallel_sync_cmd_buffers;
    std::vector<vk::CommandBuffer>   m_vk_parallel_pass_cmd_buffers;
};

} // namespace Methane::Graphics
