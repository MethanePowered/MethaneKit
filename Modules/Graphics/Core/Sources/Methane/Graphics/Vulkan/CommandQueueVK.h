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

FILE: Methane/Graphics/Vulkan/CommandQueueVK.h
Vulkan implementation of the command queue interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/CommandQueueTrackingBase.h>

#include <Tracy.hpp>
#include <vulkan/vulkan.hpp>

#include <mutex>

namespace Methane::Graphics
{

class DeviceVK;
class RenderPassVK;
class QueueFamilyReservationVK;
struct IContextVK;

class CommandQueueVK final // NOSONAR - custom destructor is required
    : public CommandQueueTrackingBase
{
public:
    struct WaitInfo
    {
        std::vector<vk::Semaphore>          semaphores;
        std::vector<vk::PipelineStageFlags> stages;
        std::vector<uint64_t>               wait_values;
    };

    CommandQueueVK(const ContextBase& context, CommandList::Type command_lists_type);
    ~CommandQueueVK() override;

    // CommandQueue interface
    uint32_t GetFamilyIndex() const noexcept override { return m_queue_family_index; }
    void Execute(CommandListSet& command_list_set, const CommandList::CompletedCallback& completed_callback = {}) override;

    // IObject interface
    bool SetName(const std::string& name) override;

    const IContextVK& GetContextVK() const noexcept;
    DeviceVK& GetDeviceVK() const noexcept;

    void WaitForSemaphore(const vk::Semaphore& semaphore, vk::PipelineStageFlags stage_flags, const uint64_t* timeline_wait_value_ptr = nullptr);
    const WaitInfo& GetWaitBeforeExecuting() const noexcept { return m_wait_before_executing; }
    const WaitInfo& GetWaitForExecutionCompleted() const;
    const WaitInfo& GetWaitForFrameExecutionCompleted(Data::Index frame_index) const;
    void ResetWaitForFrameExecution(Data::Index frame_index);

    uint32_t GetNativeQueueFamilyIndex() const noexcept { return m_queue_family_index; }
    uint32_t GetNativeQueueIndex() const noexcept       { return m_queue_index; }

    vk::Queue&       GetNativeQueue() noexcept          { return m_vk_queue; }
    const vk::Queue& GetNativeQueue() const noexcept    { return m_vk_queue; }

    vk::PipelineStageFlags GetNativeSupportedStageFlags() const noexcept    { return m_vk_supported_stage_flags; }
    vk::AccessFlags        GetNativeSupportedAccessFlags() const noexcept   { return m_vk_supported_access_flags; }

protected:
    // CommandQueueTrackingBase override
    void CompleteCommandListSetExecution(CommandListSetBase& executing_command_list_set) override;

private:
    CommandQueueVK(const ContextBase& context, CommandList::Type command_lists_type, const DeviceVK& device);
    CommandQueueVK(const ContextBase& context, CommandList::Type command_lists_type, const DeviceVK& device,
                   const QueueFamilyReservationVK& family_reservation);
    CommandQueueVK(const ContextBase& context, CommandList::Type command_lists_type, const DeviceVK& device,
                   const QueueFamilyReservationVK& family_reservation, const vk::QueueFamilyProperties& family_properties);

    void Reset();
    void AddWaitForFrameExecution(const CommandListSet& command_list_set);

    using FrameWaitInfos = std::vector<WaitInfo>;

    const uint32_t         m_queue_family_index;
    const uint32_t         m_queue_index;
    vk::Queue              m_vk_queue;
    vk::PipelineStageFlags m_vk_supported_stage_flags;
    vk::AccessFlags        m_vk_supported_access_flags;
    WaitInfo               m_wait_before_executing;
    mutable WaitInfo       m_wait_execution_completed;
    FrameWaitInfos         m_wait_frame_execution_completed;
    mutable TracyLockable(std::mutex, m_wait_frame_execution_completed_mutex)
};

} // namespace Methane::Graphics
