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

FILE: Methane/Graphics/Vulkan/System.h
Vulkan implementation of the system interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/System.h>
#include <Methane/Graphics/RHI/ICommandQueue.h>
#include <Methane/Memory.hpp>

#include <vulkan/vulkan.hpp>
#include <map>

namespace Methane::Graphics::Vulkan
{

class System final // NOSONAR - destructor is required in this class
    : public Base::System
{
public:
    System();
    ~System() override;

    // ISystem interface
    void CheckForChanges() override;
    const Ptrs<Rhi::IDevice>& UpdateGpuDevices(const Methane::Platform::AppEnvironment& app_env, const Rhi::DeviceCaps& required_device_caps) override;
    const Ptrs<Rhi::IDevice>& UpdateGpuDevices(const Rhi::DeviceCaps& required_device_caps) override;

    vk::DynamicLoader&       GetNativeLoader() noexcept       { return m_vk_loader; }
    const vk::DynamicLoader& GetNativeLoader() const noexcept { return m_vk_loader; }

    vk::Instance&       GetNativeInstance() noexcept          { return m_vk_unique_instance.get(); }
    const vk::Instance& GetNativeInstance() const noexcept    { return m_vk_unique_instance.get(); }

private:    
    vk::DynamicLoader                m_vk_loader;
    vk::UniqueInstance               m_vk_unique_instance;
    vk::UniqueDebugUtilsMessengerEXT m_vk_unique_debug_utils_messanger;
    vk::UniqueSurfaceKHR             m_vk_unique_surface;
};

} // namespace Methane::Graphics::Vulkan
