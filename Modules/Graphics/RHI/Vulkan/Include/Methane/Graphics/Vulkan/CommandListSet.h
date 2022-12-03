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

FILE: Methane/Graphics/Vulkan/CommandListSet.h
Vulkan command list set implementation.

******************************************************************************/

#pragma once

#include "ICommandList.h"

#include <Methane/Graphics/Base/CommandList.h>
#include <Methane/Data/Receiver.hpp>

#include <Tracy.hpp>

#include <vulkan/vulkan.hpp>
#include <mutex>

namespace Methane::Graphics::Vulkan
{

class CommandQueue;

class CommandListSet final
    : public Base::CommandListSet
{
public:
    explicit CommandListSet(const Refs<Rhi::ICommandList>& command_list_refs, Opt<Data::Index> frame_index_opt);

    // Base::CommandListSet interface
    void Execute(const Rhi::ICommandList::CompletedCallback& completed_callback) override;
    void WaitUntilCompleted() override;

    const std::vector<vk::CommandBuffer>& GetNativeCommandBuffers() const noexcept { return m_vk_command_buffers; }
    const vk::Semaphore&     GetNativeExecutionCompletedSemaphore() const noexcept { return m_vk_unique_execution_completed_semaphore.get(); }
    const vk::Fence&         GetNativeExecutionCompletedFence() const noexcept     { return m_vk_unique_execution_completed_fence.get(); }

    CommandQueue&       GetVulkanCommandQueue() noexcept;
    const CommandQueue& GetVulkanCommandQueue() const noexcept;

protected:
    // IObjectCallback interface
    void OnObjectNameChanged(Rhi::IObject& object, const std::string& old_name) override;

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
    TracyLockable(std::mutex,           m_vk_unique_execution_completed_fence_mutex)
};

} // namespace Methane::Graphics::Vulkan
