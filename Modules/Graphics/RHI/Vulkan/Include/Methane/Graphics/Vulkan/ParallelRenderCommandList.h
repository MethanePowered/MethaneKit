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

FILE: Methane/Graphics/Vulkan/ParallelRenderCommandList.h
Vulkan implementation of the parallel render command list interface.

******************************************************************************/

#pragma once

#include "CommandList.hpp"
#include "RenderCommandList.h"

#include <Methane/Graphics/Base/ParallelRenderCommandList.h>
#include <Methane/Graphics/Base/QueryPool.h>
#include <Methane/Data/Receiver.hpp>

namespace Methane::Graphics::Vulkan
{

class CommandQueue;
class Buffer;
class RenderPass;

class ParallelRenderCommandList final
    : public Base::ParallelRenderCommandList
    , private Data::Receiver<Rhi::IRenderPassCallback>
{
public:
    ParallelRenderCommandList(CommandQueue& command_queue, RenderPass& render_pass);

    // IParallelRenderCommandList interface
    void Reset(IDebugGroup* debug_group_ptr = nullptr) override;
    void ResetWithState(Rhi::IRenderState& render_state, IDebugGroup* debug_group_ptr = nullptr) override;
    void SetBeginningResourceBarriers(const Rhi::IResourceBarriers& resource_barriers) override;
    void SetEndingResourceBarriers(const Rhi::IResourceBarriers& resource_barriers) override;
    void SetParallelCommandListsCount(uint32_t count) override;

    // ICommandList interface
    void Commit() override;

    // Base::CommandList interface
    void Execute(const CompletedCallback& completed_callback = {}) override;
    void Complete() override;

    // IObject interface
    bool SetName(std::string_view name) override;

    const Vulkan::ICommandList& GetVulkanPrimaryCommandList() const noexcept { return m_beginning_command_list; }
    CommandQueue& GetVulkanCommandQueue() noexcept;
    RenderPass& GetVulkanRenderPass() const noexcept;

protected:
    // ParallelRenderCommandListBase interface
    [[nodiscard]] Ptr<Rhi::IRenderCommandList> CreateCommandList(bool is_beginning_list) override;

private:
    using SyncCommandList = Vulkan::CommandList<Base::CommandList, vk::PipelineBindPoint::eGraphics>;

    // IRenderPassCallback
    void OnRenderPassUpdated(const Rhi::IRenderPass& render_pass) override;

    void UpdateParallelCommandBuffers();

    RenderCommandList                m_beginning_command_list;
    vk::CommandBufferInheritanceInfo m_vk_ending_inheritance_info;
    SyncCommandList                  m_ending_command_list;
    std::vector<vk::CommandBuffer>   m_vk_parallel_sync_cmd_buffers;
    std::vector<vk::CommandBuffer>   m_vk_parallel_pass_cmd_buffers;
};

} // namespace Methane::Graphics::Vulkan
