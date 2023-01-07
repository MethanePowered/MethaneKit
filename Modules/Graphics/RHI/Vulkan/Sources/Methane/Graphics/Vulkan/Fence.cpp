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

FILE: Methane/Graphics/Vulkan/Fence.cpp
Vulkan fence implementation.

******************************************************************************/

#include <Methane/Graphics/Vulkan/Fence.h>
#include <Methane/Graphics/Vulkan/CommandQueue.h>
#include <Methane/Graphics/Vulkan/IContext.h>
#include <Methane/Graphics/Vulkan/Device.h>
#include <Methane/Graphics/Vulkan/Utils.hpp>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Instrumentation.h>

#include <nowide/convert.hpp>

namespace Methane::Graphics::Vulkan
{

static vk::UniqueSemaphore CreateTimelineSemaphore(const vk::Device& vk_device, uint64_t initial_value)
{
    META_FUNCTION_TASK();
    vk::SemaphoreTypeCreateInfo semaphore_type_create_info(vk::SemaphoreType::eTimeline, initial_value);
    return vk_device.createSemaphoreUnique(vk::SemaphoreCreateInfo().setPNext(&semaphore_type_create_info));
}

Fence::Fence(CommandQueue& command_queue)
    : Base::Fence(command_queue)
    , m_vk_device(GetVulkanCommandQueue().GetVulkanContext().GetVulkanDevice().GetNativeDevice())
    , m_vk_unique_semaphore(CreateTimelineSemaphore(m_vk_device, GetValue()))
{
    META_FUNCTION_TASK();
}

void Fence::Signal()
{
    META_FUNCTION_TASK();
    const uint64_t wait_value = GetValue();

    Base::Fence::Signal();

    const uint64_t signal_value = GetValue();
    const vk::TimelineSemaphoreSubmitInfo vk_semaphore_submit_info(wait_value, signal_value);
    vk::SubmitInfo vk_submit_info({}, {}, {}, GetNativeSemaphore());
    vk_submit_info.setPNext(&vk_semaphore_submit_info);

    GetVulkanCommandQueue().GetNativeQueue().submit(vk_submit_info);
}

void Fence::WaitOnCpu()
{
    META_FUNCTION_TASK();
    Base::Fence::WaitOnCpu();

    const uint64_t wait_value = GetValue();
    const uint64_t curr_value = m_vk_device.getSemaphoreCounterValueKHR(GetNativeSemaphore());
    if (curr_value >= wait_value) // NOSONAR - curr_value declared outside if
        return;

    META_LOG("Fence '{}' with value {} SLEEP until value {}", GetName(), curr_value, wait_value);

    const vk::SemaphoreWaitInfo wait_info(vk::SemaphoreWaitFlagBits{}, 1U, &GetNativeSemaphore(), &wait_value);
    const vk::Result semaphore_wait_result = m_vk_device.waitSemaphoresKHR(wait_info, std::numeric_limits<uint64_t>::max());
    META_CHECK_ARG_EQUAL(semaphore_wait_result, vk::Result::eSuccess);

    META_LOG("Fence '{}' AWAKE on value {}", GetName(), wait_value);
}

void Fence::WaitOnGpu(Rhi::ICommandQueue& wait_on_command_queue)
{
    META_FUNCTION_TASK();
    Base::Fence::WaitOnGpu(wait_on_command_queue);

    const uint64_t wait_value = GetValue();
    static_cast<CommandQueue&>(wait_on_command_queue).WaitForSemaphore(GetNativeSemaphore(), vk::PipelineStageFlagBits::eBottomOfPipe, &wait_value);
}

bool Fence::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Base::Fence::SetName(name))
        return false;

    SetVulkanObjectName(m_vk_device, m_vk_unique_semaphore.get(), name);
    return true;
}

CommandQueue& Fence::GetVulkanCommandQueue()
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueue&>(GetCommandQueue());
}

} // namespace Methane::Graphics::Vulkan
