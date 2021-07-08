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

FILE: Methane/Graphics/Vulkan/CommandListVK.cpp
Vulkan command lists sequence implementation

******************************************************************************/

#include "CommandListVK.h"
#include "CommandQueueVK.h"
#include "ContextVK.h"
#include "DeviceVK.h"

#include <Methane/Instrumentation.h>

#include <algorithm>

namespace Methane::Graphics
{

Ptr<CommandList::DebugGroup> CommandList::DebugGroup::Create(const std::string& name)
{
    META_FUNCTION_TASK();
    return std::make_shared<ICommandListVK::DebugGroupVK>(name);
}

ICommandListVK::DebugGroupVK::DebugGroupVK(const std::string& name)
    : CommandListBase::DebugGroupBase(name)
{
    META_FUNCTION_TASK();
}

Ptr<CommandListSet> CommandListSet::Create(const Refs<CommandList>& command_list_refs)
{
    META_FUNCTION_TASK();
    return std::make_shared<CommandListSetVK>(command_list_refs);
}

CommandListSetVK::CommandListSetVK(const Refs<CommandList>& command_list_refs)
    : CommandListSetBase(command_list_refs)
    , m_vk_device(GetCommandQueueVK().GetContextVK().GetDeviceVK().GetNativeDevice())
    , m_vk_execution_completed_fence(m_vk_device.createFence(vk::FenceCreateInfo()))
{
    META_FUNCTION_TASK();
    m_vk_command_buffers.reserve(command_list_refs.size());
    std::transform(command_list_refs.begin(), command_list_refs.end(), std::back_inserter(m_vk_command_buffers),
                   [](const Ref<CommandList>& command_list_ref)
                   {
                       return dynamic_cast<const ICommandListVK&>(command_list_ref.get()).GetNativeCommandBuffer();
                   });
}

CommandListSetVK::~CommandListSetVK()
{
    META_FUNCTION_TASK();
    m_vk_device.destroy(m_vk_execution_completed_fence);
}

void CommandListSetVK::Execute(uint32_t frame_index, const CommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    CommandListSetBase::Execute(frame_index, completed_callback);

    const CommandQueueVK& command_queue = GetCommandQueueVK();
    vk::SubmitInfo submit_info(
        command_queue.GetWaitInfo().semaphores,
        command_queue.GetWaitInfo().stages,
        m_vk_command_buffers
    );

    m_vk_device.resetFences(m_vk_execution_completed_fence);
    command_queue.GetNativeQueue().submit(submit_info, m_vk_execution_completed_fence);
}

void CommandListSetVK::WaitUntilCompleted()
{
    META_FUNCTION_TASK();
    const vk::Result execution_completed_fence_wait_result = m_vk_device.waitForFences(m_vk_execution_completed_fence, true, std::numeric_limits<uint64_t>::max());
    META_CHECK_ARG_EQUAL_DESCR(execution_completed_fence_wait_result, vk::Result::eSuccess, "failed to wait for command list set execution complete");
    Complete();
}

CommandQueueVK& CommandListSetVK::GetCommandQueueVK() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueueVK&>(GetCommandQueueBase());
}

const CommandQueueVK& CommandListSetVK::GetCommandQueueVK() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const CommandQueueVK&>(GetCommandQueueBase());
}

} // namespace Methane::Graphics
