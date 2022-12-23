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

class Device::Impl
    : public ImplWrapper<IDevice, DeviceImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_METHODS_IMPLEMENT(Device);

Device::Device(UniquePtr<Impl>&& impl_ptr)
    : Transmitter(impl_ptr->GetInterface())
    , m_impl_ptr(std::move(impl_ptr))
{
}

Device::Device(const Ptr<IDevice>& interface_ptr)
    : Device(std::make_unique<Impl>(interface_ptr))
{
}

Device::Device(IDevice& interface_ref)
    : Device(interface_ref.GetDerivedPtr<IDevice>())
{
}

IDevice& Device::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

Ptr<IDevice> Device::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterfacePtr(m_impl_ptr);
}

bool Device::SetName(std::string_view name) const
{
    return GetPrivateImpl(m_impl_ptr).SetName(name);
}

std::string_view Device::GetName() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetName();
}

const std::string& Device::GetAdapterName() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetAdapterName();
}

bool Device::IsSoftwareAdapter() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).IsSoftwareAdapter();
}

const DeviceCaps& Device::GetCapabilities() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetCapabilities();
}

std::string Device::ToString() const
{
    return GetPrivateImpl(m_impl_ptr).ToString();
}

class System::Impl : public ImplWrapper<ISystem, SystemImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_METHODS_IMPLEMENT(System);

System& System::Get()
{
    static System s_system(static_cast<Base::System&>(ISystem::Get()).GetPtr());
    return s_system;
}

System::System(const Ptr<ISystem>& interface_ptr)
    : m_impl_ptr(std::make_unique<Impl>(interface_ptr))
{
}

ISystem& System::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

void System::CheckForChanges() const
{
    GetPrivateImpl(m_impl_ptr).CheckForChanges();
}

const Devices& System::UpdateGpuDevices(const DeviceCaps& required_device_caps) const
{
    return UpdateDevices(GetPrivateImpl(m_impl_ptr).UpdateGpuDevices(required_device_caps));
}

const Devices& System::UpdateGpuDevices(const Platform::AppEnvironment& app_env,
                                        const DeviceCaps& required_device_caps) const
{
    return UpdateDevices(GetPrivateImpl(m_impl_ptr).UpdateGpuDevices(app_env, required_device_caps));
}

const Devices& System::GetGpuDevices() const META_PIMPL_NOEXCEPT
{
    return UpdateDevices(GetPrivateImpl(m_impl_ptr).GetGpuDevices());
}

Device System::GetNextGpuDevice(const Device& device) const META_PIMPL_NOEXCEPT
{
    return Device(GetPrivateImpl(m_impl_ptr).GetNextGpuDevice(device.GetInterface()));
}

Device System::GetSoftwareGpuDevice() const META_PIMPL_NOEXCEPT
{
    return Device(GetPrivateImpl(m_impl_ptr).GetSoftwareGpuDevice());
}

const DeviceCaps& System::GetDeviceCapabilities() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetDeviceCapabilities();
}

std::string System::ToString() const
{
    return GetPrivateImpl(m_impl_ptr).ToString();
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
