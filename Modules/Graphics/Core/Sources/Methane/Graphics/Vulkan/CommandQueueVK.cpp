/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/CommandQueueVK.mm
Vulkan implementation of the command queue interface.

******************************************************************************/

#include "CommandQueueVK.h"
#include "ContextVK.h"
#include "DeviceVK.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

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
    : CommandQueueBase(context, command_lists_type)
    , m_vk_queue(device.GetNativeDevice().getQueue(family_reservation.GetFamilyIndex(), family_reservation.TakeFreeQueueIndex()))
    , m_vk_command_pool(device.GetNativeDevice().createCommandPool(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(), family_reservation.GetFamilyIndex())))
{
    META_FUNCTION_TASK();
    InitializeTracyGpuContext(Tracy::GpuContext::Settings());
}

CommandQueueVK::~CommandQueueVK()
{
    META_FUNCTION_TASK();
    GetContextVK().GetDeviceVK().GetNativeDevice().destroyCommandPool(m_vk_command_pool);
}

void CommandQueueVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();

    CommandQueueBase::SetName(name);
}

const IContextVK& CommandQueueVK::GetContextVK() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const IContextVK&>(GetContextBase());
}

} // namespace Methane::Graphics
