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

FILE: Methane/Graphics/Vulkan/Fence.h
Vulkan fence implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/Fence.h>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics::Vulkan
{

class CommandQueue;

class Fence final : public Base::Fence
{
public:
    explicit Fence(CommandQueue& command_queue);

    // IFence overrides
    void Signal() override;
    void WaitOnCpu() override;
    void WaitOnGpu(ICommandQueue& wait_on_command_queue) override;

    // IObject override
    bool SetName(const std::string& name) override;

    const vk::Semaphore& GetNativeSemaphore() const noexcept { return m_vk_unique_semaphore.get(); }

private:
    CommandQueue& GetVulkanCommandQueue();

    const vk::Device&   m_vk_device;
    vk::UniqueSemaphore m_vk_unique_semaphore;
};

} // namespace Methane::Graphics::Vulkan
