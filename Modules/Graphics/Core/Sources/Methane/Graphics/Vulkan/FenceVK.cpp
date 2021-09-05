/******************************************************************************

Copyright 2020-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/FenceVK.cpp
Vulkan fence implementation.

******************************************************************************/

#include "FenceVK.h"
#include "CommandQueueVK.h"
#include "DeviceVK.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

#include <nowide/convert.hpp>

namespace Methane::Graphics
{

static vk::UniqueSemaphore CreateTimelineSemaphore(const vk::Device& vk_device, uint64_t initial_value)
{
    META_FUNCTION_TASK();
    vk::SemaphoreTypeCreateInfo semaphore_type_create_info(vk::SemaphoreType::eTimeline, initial_value);
    return vk_device.createSemaphoreUnique(vk::SemaphoreCreateInfo().setPNext(&semaphore_type_create_info));
}

Ptr<Fence> Fence::Create(CommandQueue& command_queue)
{
    META_FUNCTION_TASK();
    return std::make_shared<FenceVK>(static_cast<CommandQueueVK&>(command_queue));
}

FenceVK::FenceVK(CommandQueueVK& command_queue)
    : FenceBase(command_queue)
    , m_vk_device(GetCommandQueueVK().GetContextVK().GetDeviceVK().GetNativeDevice())
    , m_vk_unique_semaphore(CreateTimelineSemaphore(m_vk_device, GetValue()))
{
    META_FUNCTION_TASK();
}

void FenceVK::Signal()
{
    META_FUNCTION_TASK();
    const uint64_t wait_value = GetValue();

    FenceBase::Signal();

    const uint64_t signal_value = GetValue();
    const vk::TimelineSemaphoreSubmitInfo vk_semaphore_submit_info(wait_value, signal_value);
    vk::SubmitInfo vk_submit_info({}, {}, {}, GetNativeSemaphore());
    vk_submit_info.setPNext(&vk_semaphore_submit_info);

    GetCommandQueueVK().GetNativeQueue().submit(vk_submit_info);
}

void FenceVK::WaitOnCpu()
{
    META_FUNCTION_TASK();
    FenceBase::WaitOnCpu();

    const uint64_t wait_value = GetValue();
    const uint64_t curr_value = m_vk_device.getSemaphoreCounterValueKHR(GetNativeSemaphore());
    if (curr_value >= wait_value)
        return;

    META_LOG("Fence '{}' with value {} SLEEP until value {}", GetName(), curr_value, wait_value);

    const vk::SemaphoreWaitInfo wait_info(vk::SemaphoreWaitFlagBits{}, 1U, &GetNativeSemaphore(), &wait_value);
    const vk::Result semaphore_wait_result = m_vk_device.waitSemaphoresKHR(wait_info, std::numeric_limits<uint64_t>::max());
    META_CHECK_ARG_EQUAL(semaphore_wait_result, vk::Result::eSuccess);

    META_LOG("Fence '{}' AWAKE on value {}", GetName(), wait_value);
}

void FenceVK::WaitOnGpu(CommandQueue& wait_on_command_queue)
{
    META_FUNCTION_TASK();
    FenceBase::WaitOnGpu(wait_on_command_queue);
    static_cast<CommandQueueVK&>(wait_on_command_queue).WaitForSemaphore(GetNativeSemaphore(), vk::PipelineStageFlagBits::eTopOfPipe);
}

void FenceVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (ObjectBase::GetName() == name)
        return;

   ObjectBase::SetName(name);
}

CommandQueueVK& FenceVK::GetCommandQueueVK()
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueueVK&>(GetCommandQueue());
}

} // namespace Methane::Graphics
