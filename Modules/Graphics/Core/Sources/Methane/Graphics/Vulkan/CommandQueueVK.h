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

FILE: Methane/Graphics/Vulkan/CommandQueueVK.h
Vulkan implementation of the command queue interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/CommandQueueBase.h>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics
{

class DeviceVK;
class RenderPassVK;
class QueueFamilyReservationVK;
struct IContextVK;

class CommandQueueVK final : public CommandQueueBase
{
public:
    CommandQueueVK(const ContextBase& context, CommandList::Type command_lists_type);
    ~CommandQueueVK() override;

    // Object interface
    void SetName(const std::string& name) override;
    
    const IContextVK& GetContextVK() const noexcept;

    vk::Queue&       GetNativeQueue()       { return m_vk_queue; }
    const vk::Queue& GetNativeQueue() const { return m_vk_queue; }

    vk::CommandPool& GetNativeCommandPool() { return m_vk_command_pool; }
    const vk::CommandPool& GetNativeCommandPool() const { return m_vk_command_pool; }

private:
    CommandQueueVK(const ContextBase& context, CommandList::Type command_lists_type,
                   const DeviceVK& device);
    CommandQueueVK(const ContextBase& context, CommandList::Type command_lists_type,
                   const DeviceVK& device, const QueueFamilyReservationVK& family_reservation);

    void Reset();

    vk::Queue       m_vk_queue;
    vk::CommandPool m_vk_command_pool;
};

} // namespace Methane::Graphics
