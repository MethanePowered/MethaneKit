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

FILE: Methane/Graphics/Vulkan/DeviceVK.h
Vulkan implementation of the device interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/DeviceBase.h>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics
{

class DeviceVK final : public DeviceBase // NOSONAR
{
public:
    static Device::Features GetSupportedFeatures(const vk::PhysicalDevice& vk_physical_device);

    explicit DeviceVK(const vk::PhysicalDevice& vk_physical_device, const std::vector<vk::DeviceQueueCreateInfo>& vk_queue_create_infos);
    ~DeviceVK() override;

    const vk::PhysicalDevice& GetNativePhysicalDevice() const noexcept { return m_vk_physical_device; }
    const vk::Device&         GetNativeDevice() const noexcept         { return m_vk_device; }

private:
    vk::PhysicalDevice m_vk_physical_device;
    vk::Device         m_vk_device;
};

class SystemVK final : public SystemBase // NOSONAR
{
public:
    SystemVK();
    ~SystemVK() override;

    // System interface
    void CheckForChanges() override;
    const Ptrs<Device>& UpdateGpuDevices(const Platform::AppEnvironment& app_env, const Device::Capabilities& required_device_caps) override;
    const Ptrs<Device>& UpdateGpuDevices(const Device::Capabilities& required_device_caps) override;

    vk::DynamicLoader&       GetNativeLoader() noexcept       { return m_vk_loader; }
    const vk::DynamicLoader& GetNativeLoader() const noexcept { return m_vk_loader; }

    vk::Instance&       GetNativeInstance() noexcept          { return m_vk_instance; }
    const vk::Instance& GetNativeInstance() const noexcept    { return m_vk_instance; }

    const vk::SurfaceKHR& GetNativeSurface() const noexcept   { return m_vk_surface; }

private:
    void AddDevice(const vk::PhysicalDevice& vk_physical_device);

    vk::DynamicLoader m_vk_loader;
    vk::Instance      m_vk_instance;
    vk::SurfaceKHR    m_vk_surface;
};

} // namespace Methane::Graphics
