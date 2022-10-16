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

static vk::PipelineStageFlags GetPipelineStageFlagsByQueueFlags(vk::QueueFlags vk_queue_flags)
{
    META_FUNCTION_TASK();
    vk::PipelineStageFlags vk_pipeline_stage_flags = vk::PipelineStageFlagBits::eTopOfPipe |
                                                     vk::PipelineStageFlagBits::eBottomOfPipe;

    if (vk_queue_flags & vk::QueueFlagBits::eGraphics)
        vk_pipeline_stage_flags |= vk::PipelineStageFlagBits::eAllGraphics
                                |  vk::PipelineStageFlagBits::eDrawIndirect
                                |  vk::PipelineStageFlagBits::eVertexInput
                                |  vk::PipelineStageFlagBits::eVertexShader
                                |  vk::PipelineStageFlagBits::eTessellationControlShader
                                |  vk::PipelineStageFlagBits::eTessellationEvaluationShader
                                |  vk::PipelineStageFlagBits::eGeometryShader
                                |  vk::PipelineStageFlagBits::eFragmentShader
                                |  vk::PipelineStageFlagBits::eEarlyFragmentTests
                                |  vk::PipelineStageFlagBits::eLateFragmentTests
                                |  vk::PipelineStageFlagBits::eColorAttachmentOutput;

    if (vk_queue_flags & vk::QueueFlagBits::eCompute)
        vk_pipeline_stage_flags |= vk::PipelineStageFlagBits::eComputeShader;

    if (vk_queue_flags & vk::QueueFlagBits::eTransfer)
        vk_pipeline_stage_flags |= vk::PipelineStageFlagBits::eTransfer;

    return vk_pipeline_stage_flags;
}

static vk::AccessFlags GetAccessFlagsByQueueFlags(vk::QueueFlags vk_queue_flags)
{
    META_FUNCTION_TASK();
    vk::AccessFlags vk_access_flags = vk::AccessFlagBits::eHostRead
                                    | vk::AccessFlagBits::eHostWrite
                                    | vk::AccessFlagBits::eMemoryRead
                                    | vk::AccessFlagBits::eMemoryWrite;

    if (vk_queue_flags & vk::QueueFlagBits::eGraphics)
        vk_access_flags |= vk::AccessFlagBits::eIndirectCommandRead
                        |  vk::AccessFlagBits::eIndexRead
                        |  vk::AccessFlagBits::eVertexAttributeRead
                        |  vk::AccessFlagBits::eUniformRead
                        |  vk::AccessFlagBits::eInputAttachmentRead
                        |  vk::AccessFlagBits::eColorAttachmentRead
                        |  vk::AccessFlagBits::eColorAttachmentWrite
                        |  vk::AccessFlagBits::eDepthStencilAttachmentRead
                        |  vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    if (vk_queue_flags & vk::QueueFlagBits::eCompute ||
        vk_queue_flags & vk::QueueFlagBits::eGraphics)
        vk_access_flags |= vk::AccessFlagBits::eShaderRead
                        |  vk::AccessFlagBits::eShaderWrite;

    if (vk_queue_flags & vk::QueueFlagBits::eTransfer)
        vk_access_flags |= vk::AccessFlagBits::eTransferRead
                        |  vk::AccessFlagBits::eTransferWrite;

    return vk_access_flags;
}

Ptr<CommandQueue> CommandQueue::Create(const Context& context, CommandList::Type command_lists_type)
{
    META_FUNCTION_TASK();
    auto command_queue_ptr = std::make_shared<CommandQueueVK>(dynamic_cast<const ContextBase&>(context), command_lists_type);
#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
    // TimestampQueryPoolBase construction uses command queue and requires it to be fully constructed
    command_queue_ptr->InitializeTimestampQueryPool();
#endif
    return command_queue_ptr;
}

CommandQueueVK::CommandQueueVK(const ContextBase& context, CommandList::Type command_lists_type)
    : CommandQueueVK(context, command_lists_type, static_cast<const IContextVK&>(context).GetDeviceVK())
{
    META_FUNCTION_TASK();
}

CommandQueueVK::CommandQueueVK(const ContextBase& context, CommandList::Type command_lists_type, const DeviceVK& device)
    : CommandQueueVK(context, command_lists_type, device,
                     device.GetQueueFamilyReservation(command_lists_type))
{
    META_FUNCTION_TASK();
}

CommandQueueVK::CommandQueueVK(const ContextBase& context, CommandList::Type command_lists_type, const DeviceVK& device,
                               const QueueFamilyReservationVK& family_reservation)
    : CommandQueueVK(context, command_lists_type, device, family_reservation,
                     device.GetNativeQueueFamilyProperties(family_reservation.GetFamilyIndex()))
{
    META_FUNCTION_TASK();
}

CommandQueueVK::CommandQueueVK(const ContextBase& context, CommandList::Type command_lists_type, const DeviceVK& device,
                               const QueueFamilyReservationVK& family_reservation, const vk::QueueFamilyProperties& family_properties)
    : CommandQueueTrackingBase(context, command_lists_type)
    , m_queue_family_index(family_reservation.GetFamilyIndex())
    , m_queue_index(family_reservation.ClaimQueueIndex())
    , m_vk_queue(device.GetNativeDevice().getQueue(m_queue_family_index, m_queue_index))
    , m_vk_supported_stage_flags(GetPipelineStageFlagsByQueueFlags(family_properties.queueFlags))
    , m_vk_supported_access_flags(GetAccessFlagsByQueueFlags(family_properties.queueFlags))
{
    META_FUNCTION_TASK();
}

CommandQueueVK::~CommandQueueVK()
{
    META_FUNCTION_TASK();
    ShutdownQueueExecution();
    GetDeviceVK().GetQueueFamilyReservation(CommandQueueBase::GetCommandListType()).ReleaseQueueIndex(m_queue_index);
}

void CommandQueueVK::Execute(CommandListSet& command_list_set, const CommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();

    AddWaitForFrameExecution(command_list_set);
    CommandQueueTrackingBase::Execute(command_list_set, completed_callback);

    m_wait_before_executing.semaphores.clear();
    m_wait_before_executing.stages.clear();
    m_wait_before_executing.wait_values.clear();
}

void CommandQueueVK::WaitForSemaphore(const vk::Semaphore& semaphore, vk::PipelineStageFlags stage_flags, const uint64_t* timeline_wait_value_ptr)
{
    META_FUNCTION_TASK();
    m_wait_before_executing.semaphores.emplace_back(semaphore);
    m_wait_before_executing.stages.emplace_back(stage_flags);

    const bool no_timeline_waits = m_wait_before_executing.wait_values.empty();
    if (timeline_wait_value_ptr && no_timeline_waits)
    {
        m_wait_before_executing.wait_values.resize(m_wait_before_executing.semaphores.size() - 1U, 0U);
    }
    if (timeline_wait_value_ptr || !no_timeline_waits)
    {
        m_wait_before_executing.wait_values.push_back(timeline_wait_value_ptr ? *timeline_wait_value_ptr : 0U);
    }
}

const CommandQueueVK::WaitInfo& CommandQueueVK::GetWaitForExecutionCompleted() const
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
        m_wait_execution_completed.semaphores.emplace_back(executing_command_list_set.GetNativeExecutionCompletedSemaphore());
        executing_command_list_sets.pop();
    }

    m_wait_execution_completed.stages.resize(m_wait_execution_completed.semaphores.size(), vk::PipelineStageFlagBits::eBottomOfPipe);
    return m_wait_execution_completed;
}

const CommandQueueVK::WaitInfo& CommandQueueVK::GetWaitForFrameExecutionCompleted(Data::Index frame_index) const
{
    META_FUNCTION_TASK();
    static const CommandQueueVK::WaitInfo s_empty_wait_info;

    std::scoped_lock lock_guard(m_wait_frame_execution_completed_mutex);
    return frame_index < m_wait_frame_execution_completed.size()
         ? m_wait_frame_execution_completed[frame_index]
         : s_empty_wait_info;
}

void CommandQueueVK::ResetWaitForFrameExecution(Data::Index frame_index)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_wait_frame_execution_completed_mutex);
    if (frame_index >= m_wait_frame_execution_completed.size())
        return;

    WaitInfo& wait_info = m_wait_frame_execution_completed[frame_index];
    wait_info.semaphores.clear();
    wait_info.stages.clear();
}

void CommandQueueVK::AddWaitForFrameExecution(const CommandListSet& command_list_set)
{
    META_FUNCTION_TASK();
    if (GetCommandListType() != CommandList::Type::Render)
        return;

    const auto& vulkan_command_list_set = static_cast<const CommandListSetVK&>(command_list_set);
    const Data::Index wait_info_index = command_list_set.GetFrameIndex().value_or(0U);

    std::scoped_lock lock_guard(m_wait_frame_execution_completed_mutex);

    m_wait_frame_execution_completed.resize(wait_info_index + 1U);
    WaitInfo& frame_wait_info = m_wait_frame_execution_completed[wait_info_index];
    frame_wait_info.semaphores.emplace_back(vulkan_command_list_set.GetNativeExecutionCompletedSemaphore());
    frame_wait_info.stages.emplace_back(vk::PipelineStageFlagBits::eBottomOfPipe);
}

void CommandQueueVK::CompleteCommandListSetExecution(CommandListSetBase& executing_command_list_set)
{
    META_FUNCTION_TASK();
    ResetWaitForFrameExecution(executing_command_list_set.GetFrameIndex().value_or(0U));
    CommandQueueTrackingBase::CompleteCommandListSetExecution(executing_command_list_set);
}

bool CommandQueueVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!CommandQueueTrackingBase::SetName(name))
        return false;

    const vk::Device& vk_device = GetDeviceVK().GetNativeDevice();
    SetVulkanObjectName(vk_device, m_vk_queue, name);
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
