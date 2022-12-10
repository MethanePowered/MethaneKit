/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/Device.cpp
Methane System and Device PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/Device.h>

#if defined METHANE_GFX_DIRECTX

#include <Methane/Graphics/DirectX/Device.h>
using DeviceImpl = Methane::Graphics::DirectX::Device;
using SystemImpl = Methane::Graphics::DirectX::System;

#elif defined METHANE_GFX_VULKAN

#include <Methane/Graphics/Vulkan/Device.h>
using DeviceImpl = Methane::Graphics::Vulkan::Device;
using SystemImpl = Methane::Graphics::Vulkan::System;

#elif defined METHANE_GFX_METAL

#include <Methane/Graphics/Metal/Device.hh>
using DeviceImpl = Methane::Graphics::Metal::Device;
using SystemImpl = Methane::Graphics::Metal::System;

#else // METHAN_GFX_[API] is undefined

static_assert(false, "Static graphics API macro-definition is missing.");

#endif

#include <Methane/Instrumentation.h>

#include "ImplWrapper.hpp"

#include <algorithm>

namespace Methane::Graphics::Rhi
{

class Device::Impl : public ImplWrapper<IDevice, DeviceImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

Device::Device(const Ptr <IDevice>& interface_ptr)
    : m_impl_ptr(std::make_unique<Impl>(interface_ptr))
{
}

IDevice& Device::GetInterface() const noexcept
{
    return m_impl_ptr->GetInterface();
}

const std::string& Device::GetAdapterName() const noexcept
{
    return m_impl_ptr->Get().GetAdapterName();
}

bool Device::IsSoftwareAdapter() const noexcept
{
    return m_impl_ptr->Get().IsSoftwareAdapter();
}

const DeviceCaps& Device::GetCapabilities() const noexcept
{
    return m_impl_ptr->Get().GetCapabilities();
}

std::string Device::ToString() const
{
    return m_impl_ptr->Get().ToString();
}

bool Device::SetName(const std::string& name) const
{
    return m_impl_ptr->Get().SetName(name);
}

const std::string& Device::GetName() const noexcept
{
    return m_impl_ptr->Get().GetName();
}

class System::Impl : public ImplWrapper<ISystem, SystemImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

System& System::Get()
{
    static System s_system(static_cast<Base::System&>(ISystem::Get()).GetPtr());
    return s_system;
}

System::System(const Ptr<ISystem>& interface_ptr)
    : m_impl_ptr(std::make_unique<Impl>(interface_ptr))
{
}

ISystem& System::GetInterface() const noexcept
{
    return m_impl_ptr->GetInterface();
}

void System::CheckForChanges() const
{
    m_impl_ptr->Get().CheckForChanges();
}

const Devices& System::UpdateGpuDevices(const DeviceCaps& required_device_caps) const
{
    return UpdateDevices(m_impl_ptr->Get().UpdateGpuDevices(required_device_caps));
}

const Devices& System::UpdateGpuDevices(const Platform::AppEnvironment& app_env,
                                        const DeviceCaps& required_device_caps) const
{
    return UpdateDevices(m_impl_ptr->Get().UpdateGpuDevices(app_env, required_device_caps));
}

const Devices& System::GetGpuDevices() const noexcept
{
    return UpdateDevices(m_impl_ptr->Get().GetGpuDevices());
}

Device System::GetNextGpuDevice(const Device& device) const noexcept
{
    return Device(m_impl_ptr->Get().GetNextGpuDevice(device.GetInterface()));
}

Device System::GetSoftwareGpuDevice() const noexcept
{
    return Device(m_impl_ptr->Get().GetSoftwareGpuDevice());
}

const DeviceCaps& System::GetDeviceCapabilities() const noexcept
{
    return m_impl_ptr->Get().GetDeviceCapabilities();
}

std::string System::ToString() const
{
    return m_impl_ptr->Get().ToString();
}

const Devices& System::UpdateDevices(const Ptrs<Rhi::IDevice>& devices) const
{
    META_FUNCTION_TASK();
    m_devices.clear();
    std::transform(devices.cbegin(), devices.cend(), std::back_inserter(m_devices),
                   [](const Ptr<Rhi::IDevice>& device_ptr)
                   { return Device(device_ptr); });
    return m_devices;
}

} // namespace Methane::Graphics::Rhi
