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

FILE: Methane/Graphics/DeviceBase.cpp
Base implementation of the device interface.

******************************************************************************/

#include "DeviceBase.h"

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <sstream>

namespace Methane::Graphics
{

DeviceBase::DeviceBase(const std::string& adapter_name, bool is_software_adapter, Features supported_features)
    : m_adapter_name(adapter_name)
    , m_is_software_adapter(is_software_adapter)
    , m_supported_features(supported_features)
{
    META_FUNCTION_TASK();
}

std::string DeviceBase::ToString() const
{
    META_FUNCTION_TASK();
    return fmt::format("GPU \"{}\"", GetAdapterName());
}

void DeviceBase::OnRemovalRequested()
{
    META_FUNCTION_TASK();
    Emit(&IDeviceCallback::OnDeviceRemovalRequested, std::ref(*this));
}

void DeviceBase::OnRemoved()
{
    META_FUNCTION_TASK();
    Emit(&IDeviceCallback::OnDeviceRemoved, std::ref(*this));
}

void SystemBase::RequestRemoveDevice(Device& device) const
{
    META_FUNCTION_TASK();
    static_cast<DeviceBase&>(device).OnRemovalRequested();
}

void SystemBase::RemoveDevice(Device& device)
{
    META_FUNCTION_TASK();
    const auto device_it = std::find_if(m_devices.begin(), m_devices.end(),
        [&device](const Ptr<Device>& device_ptr)
        { return std::addressof(device) == std::addressof(*device_ptr); }
    );
    if (device_it == m_devices.end())
        return;

    Ptr<Device> device_ptr = *device_it;
    m_devices.erase(device_it);
    static_cast<DeviceBase&>(device).OnRemoved();
}

Ptr<Device> SystemBase::GetNextGpuDevice(const Device& device) const
{
    META_FUNCTION_TASK();
    Ptr<Device> next_device_ptr;
    
    if (m_devices.empty())
        return next_device_ptr;
    
    auto device_it = std::find_if(m_devices.begin(), m_devices.end(),
                                  [&device](const Ptr<Device>& system_device_ptr)
                                  { return std::addressof(device) == system_device_ptr.get(); });
    if (device_it == m_devices.end())
        return next_device_ptr;
    
    return device_it == m_devices.end() - 1 ? m_devices.front() : *(device_it + 1);
}

Ptr<Device> SystemBase::GetSoftwareGpuDevice() const
{
    META_FUNCTION_TASK();
    auto sw_device_it = std::find_if(m_devices.begin(), m_devices.end(),
        [](const Ptr<Device>& system_device_ptr)
        { return system_device_ptr && system_device_ptr->IsSoftwareAdapter(); });

    return sw_device_it != m_devices.end() ? *sw_device_it : Ptr<Device>();
}

std::string SystemBase::ToString() const
{
    META_FUNCTION_TASK();
    std::stringstream ss;
    ss << "Available graphics devices:" << std::endl;
    for(const Ptr<Device>& device_ptr : m_devices)
    {
        META_CHECK_ARG_NOT_NULL(device_ptr);
        ss << "  - " << device_ptr->ToString() << ";" << std::endl;
    }
    return ss.str();
}

} // namespace Methane::Graphics