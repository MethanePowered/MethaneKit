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

#include "Pimpl.hpp"

#ifdef META_GFX_METAL
#include <Device.hh>
#else
#include <Device.h>
#endif

#include <algorithm>
#include <iterator>

namespace Methane::Graphics::Rhi
{

META_PIMPL_METHODS_IMPLEMENT(Device);

Device::Device(Ptr<Impl>&& impl_ptr)
    : m_impl_ptr(std::move(impl_ptr))
{
}

Device::Device(const Ptr<IDevice>& interface_ptr)
    : Device(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

Device::Device(IDevice& interface_ref)
    : Device(interface_ref.GetDerivedPtr<IDevice>())
{
}

IDevice& Device::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<IDevice> Device::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool Device::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view Device::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

const std::string& Device::GetAdapterName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetAdapterName();
}

bool Device::IsSoftwareAdapter() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).IsSoftwareAdapter();
}

const DeviceCaps& Device::GetCapabilities() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetCapabilities();
}

std::string Device::ToString() const
{
    return GetImpl(m_impl_ptr).ToString();
}

void Device::Connect(Data::Receiver<IDeviceCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IDeviceCallback>::Connect(receiver);
}

void Device::Disconnect(Data::Receiver<IDeviceCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IDeviceCallback>::Disconnect(receiver);
}

META_PIMPL_METHODS_IMPLEMENT(System);

NativeApi System::GetNativeApi() noexcept
{
    return ISystem::GetNativeApi();
}

System& System::Get()
{
    static System s_system(static_cast<Base::System&>(ISystem::Get()).GetPtr());
    return s_system;
}

System::System(const Ptr<ISystem>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

ISystem& System::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

void System::CheckForChanges() const
{
    GetImpl(m_impl_ptr).CheckForChanges();
}

const Devices& System::UpdateGpuDevices(const DeviceCaps& required_device_caps) const
{
    return UpdateDevices(GetImpl(m_impl_ptr).UpdateGpuDevices(required_device_caps));
}

const Devices& System::UpdateGpuDevices(const Platform::AppEnvironment& app_env,
                                        const DeviceCaps& required_device_caps) const
{
    return UpdateDevices(GetImpl(m_impl_ptr).UpdateGpuDevices(app_env, required_device_caps));
}

const Devices& System::GetGpuDevices() const META_PIMPL_NOEXCEPT
{
    return UpdateDevices(GetImpl(m_impl_ptr).GetGpuDevices());
}

Device System::GetNextGpuDevice(const Device& device) const META_PIMPL_NOEXCEPT
{
    return Device(GetImpl(m_impl_ptr).GetNextGpuDevice(device.GetInterface()));
}

Device System::GetSoftwareGpuDevice() const META_PIMPL_NOEXCEPT
{
    return Device(GetImpl(m_impl_ptr).GetSoftwareGpuDevice());
}

const DeviceCaps& System::GetDeviceCapabilities() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetDeviceCapabilities();
}

std::string System::ToString() const
{
    return GetImpl(m_impl_ptr).ToString();
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
