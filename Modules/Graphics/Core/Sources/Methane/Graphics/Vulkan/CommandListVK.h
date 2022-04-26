/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/CommandListVK.h
Vulkan command lists sequence implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/CommandListBase.h>
#include <Methane/Data/Receiver.hpp>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics
{

class CommandQueueVK;

struct ICommandListVK
{
    class DebugGroupVK final : public CommandListBase::DebugGroupBase
    {
    public:
        explicit DebugGroupVK(const std::string& name);

        const vk::DebugUtilsLabelEXT& GetNativeDebugLabel() const noexcept { return m_vk_debug_label; }

    private:
        vk::DebugUtilsLabelEXT m_vk_debug_label;
    };

    enum class CommandBufferType : uint32_t
    {
        Primary,             // Primary command buffer with no-render commands, like pipeline barriers, executed before render pass begin
        SecondaryRenderPass, // Secondary command buffer with render pass only commands, excluding pipeline barriers
    };

    virtual CommandQueueVK&          GetCommandQueueVK() = 0;
    virtual const CommandQueueVK&    GetCommandQueueVK() const = 0;
    virtual const vk::CommandBuffer& GetNativeCommandBufferDefault() const = 0;
    virtual const vk::CommandBuffer& GetNativeCommandBuffer(CommandBufferType cmd_buffer_type = CommandBufferType::Primary) const = 0;
    virtual vk::PipelineBindPoint    GetNativePipelineBindPoint() const = 0;
    virtual void SetResourceBarriers(const Resource::Barriers& resource_barriers) = 0;

    virtual ~ICommandListVK() = default;
};

class CommandListSetVK final
    : public CommandListSetBase
{
public:
    explicit CommandListSetVK(const Refs<CommandList>& command_list_refs, Opt<Data::Index> frame_index_opt);

    // CommandListSetBase interface
    void Execute(const CommandList::CompletedCallback& completed_callback) override;
    void WaitUntilCompleted() override;

    const std::vector<vk::CommandBuffer>& GetNativeCommandBuffers() const noexcept { return m_vk_command_buffers; }
    const vk::Semaphore&     GetNativeExecutionCompletedSemaphore() const noexcept { return m_vk_unique_execution_completed_semaphore.get(); }
    const vk::Fence&         GetNativeExecutionCompletedFence() const noexcept     { return m_vk_unique_execution_completed_fence.get(); }

    CommandQueueVK&       GetCommandQueueVK() noexcept;
    const CommandQueueVK& GetCommandQueueVK() const noexcept;

protected:
    // IObjectCallback interface
    void OnObjectNameChanged(Object& object, const std::string& old_name) override;

private:
    const std::vector<vk::Semaphore>&          GetWaitSemaphores();
    const std::vector<vk::PipelineStageFlags>& GetWaitStages();
    const std::vector<uint64_t>&               GetWaitValues();
    void UpdateNativeDebugName();

    const vk::PipelineStageFlags        m_vk_wait_frame_buffer_rendering_on_stages;
    const vk::Device&                   m_vk_device;
    std::vector<vk::CommandBuffer>      m_vk_command_buffers;
    std::vector<vk::Semaphore>          m_vk_wait_semaphores;
    std::vector<vk::PipelineStageFlags> m_vk_wait_stages;
    std::vector<uint64_t>               m_vk_wait_values;
    vk::UniqueSemaphore                 m_vk_unique_execution_completed_semaphore;
    vk::UniqueFence                     m_vk_unique_execution_completed_fence;
};

} // namespace Methane::Graphics
