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

FILE: Methane/Graphics/Vulkan/CommandQueueVK.cpp
Vulkan implementation of the command queue interface.

******************************************************************************/

#include "CommandQueueVK.h"
#include "CommandListVK.h"
#include "ContextVK.h"
#include "DeviceVK.h"
#include "UtilsVK.hpp"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

#include <fmt/format.h>

namespace Methane::Graphics
{

static vk::UniqueCommandPool CreateVulkanCommandPool(const vk::Device& vk_device, uint32_t queue_family_index)
{
    META_FUNCTION_TASK();
    vk::CommandPoolCreateInfo vk_command_pool_info(vk::CommandPoolCreateFlags(), queue_family_index);
    vk_command_pool_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    return vk_device.createCommandPoolUnique(vk_command_pool_info);
}

Ptr<CommandQueue> CommandQueue::Create(const Context& context, CommandList::Type command_lists_type)
{
    META_FUNCTION_TASK();
    return std::make_shared<CommandQueueVK>(dynamic_cast<const ContextBase&>(context), command_lists_type);
}

CommandQueueVK::CommandQueueVK(const ContextBase& context, CommandList::Type command_lists_type)
    : CommandQueueVK(context, command_lists_type, static_cast<const IContextVK&>(context).GetDeviceVK())
{
    META_FUNCTION_TASK();
}

CommandQueueVK::CommandQueueVK(const ContextBase& context, CommandList::Type command_lists_type,
                               const DeviceVK& device)
    : CommandQueueVK(context, command_lists_type, device, device.GetQueueFamilyReservation(command_lists_type))
{
    META_FUNCTION_TASK();
}

CommandQueueVK::CommandQueueVK(const ContextBase& context, CommandList::Type command_lists_type,
                               const DeviceVK& device, const QueueFamilyReservationVK& family_reservation)
    : CommandQueueTrackingBase(context, command_lists_type)
    , m_queue_family_index(family_reservation.GetFamilyIndex())
    , m_queue_index(family_reservation.ClaimQueueIndex())
    , m_vk_queue(device.GetNativeDevice().getQueue(m_queue_family_index, m_queue_index))
    , m_vk_unique_command_pool(CreateVulkanCommandPool(device.GetNativeDevice(), m_queue_family_index))
{
    META_FUNCTION_TASK();
    InitializeTracyGpuContext(Tracy::GpuContext::Settings());
}

CommandQueueVK::~CommandQueueVK()
{
    META_FUNCTION_TASK();
    GetDeviceVK().GetQueueFamilyReservation(CommandQueueBase::GetCommandListType()).ReleaseQueueIndex(m_queue_index);
}

void CommandQueueVK::Execute(CommandListSet& command_lists, const CommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    CommandQueueTrackingBase::Execute(command_lists, completed_callback);

    m_wait_before_executing.semaphores.clear();
    m_wait_before_executing.stages.clear();
};

void CommandQueueVK::WaitForSemaphore(const vk::Semaphore& semaphore, vk::PipelineStageFlags stage_flags)
{
    META_FUNCTION_TASK();
    m_wait_before_executing.semaphores.emplace_back(semaphore);
    m_wait_before_executing.stages.emplace_back(stage_flags);
}

const CommandQueueVK::WaitInfo& CommandQueueVK::GetWaitForExecutionCompleted(const Opt<Data::Index>& frame_index_opt) const
{
    META_FUNCTION_TASK();
    const auto           executing_command_lists_guard = GetExecutingCommandListsGuard();
    CommandListSetsQueue executing_command_list_sets   = executing_command_lists_guard.GetCommandListsQueue(); // copy the queue for iterating

    m_wait_execution_completed.semaphores.clear();
    m_wait_execution_completed.semaphores.reserve(executing_command_list_sets.size());

    while(!executing_command_list_sets.empty())
    {
        META_CHECK_ARG_NOT_NULL(executing_command_list_sets.front());
        const auto& executing_command_list_set = static_cast<const CommandListSetVK&>(*executing_command_list_sets.front());
        if (frame_index_opt && executing_command_list_set.GetExecutingOnFrameIndex() != *frame_index_opt)
        {
            executing_command_list_sets.pop();
            continue;
        }

        m_wait_execution_completed.semaphores.emplace_back(executing_command_list_set.GetNativeExecutionCompletedSemaphore());
        executing_command_list_sets.pop();
    }

    m_wait_execution_completed.stages.resize(m_wait_execution_completed.semaphores.size(), vk::PipelineStageFlagBits::eTopOfPipe);
    return m_wait_execution_completed;
}

bool CommandQueueVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!CommandQueueBase::SetName(name))
        return false;

    const vk::Device& vk_device = GetDeviceVK().GetNativeDevice();
    SetVulkanObjectName(vk_device, m_vk_queue, name);
    SetVulkanObjectName(vk_device, m_vk_unique_command_pool.get(), fmt::format("{} Command Pool", name));
    return true;
}

const IContextVK& CommandQueueVK::GetContextVK() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const IContextVK&>(GetContextBase());
}

DeviceVK& CommandQueueVK::GetDeviceVK() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<DeviceVK&>(GetDeviceBase());
}

} // namespace Methane::Graphics
