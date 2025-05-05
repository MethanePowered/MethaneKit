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

FILE: Methane/Graphics/RHI/System.cpp
Methane System PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/System.h>

#include <Methane/Pimpl.hpp>

#ifdef META_GFX_METAL
#include <System.hh>
#else
#include <System.h>
#endif

#include <algorithm>
#include <iterator>

namespace Methane::Graphics::Rhi
{

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
    m_devices.clear();
    std::ranges::transform(devices, std::back_inserter(m_devices),
                           [](const Ptr<Rhi::IDevice>& device_ptr)
                           { return Device(device_ptr); });
    return m_devices;
}

} // namespace Methane::Graphics::Rhi
