/******************************************************************************

Copyright 2019-2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Base/System.cpp
Base implementation of the device interface.

******************************************************************************/

#include <Methane/Graphics/Base/System.h>
#include <Methane/Graphics/Base/Device.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <sstream>

namespace Methane::Graphics::Base
{

void System::AddDevice(Ptr<Rhi::IDevice> device_ptr)
{
    META_FUNCTION_TASK();
    if (const Rhi::DeviceFeatureMask device_supported_features = device_ptr->GetCapabilities().features;
        !GetDeviceCapabilities().features.HasBits(device_supported_features))
        return;

    m_devices.emplace_back(std::move(device_ptr));
}

void System::RequestRemoveDevice(Rhi::IDevice& device) const
{
    META_FUNCTION_TASK();
    static_cast<Device&>(device).OnRemovalRequested();
}

void System::RemoveDevice(Rhi::IDevice& device)
{
    META_FUNCTION_TASK();
    const auto device_it = std::find_if(m_devices.begin(), m_devices.end(),
        [&device](const Ptr<Rhi::IDevice>& device_ptr)
        { return std::addressof(device) == std::addressof(*device_ptr); }
    );
    if (device_it == m_devices.end())
        return;

    Ptr<Rhi::IDevice> device_ptr = *device_it;
    m_devices.erase(device_it);
    static_cast<Device&>(device).OnRemoved();
}

Ptr<Rhi::IDevice> System::GetNextGpuDevice(const Rhi::IDevice& device) const noexcept
{
    META_FUNCTION_TASK();
    Ptr<Rhi::IDevice> next_device_ptr;
    
    if (m_devices.empty())
        return next_device_ptr;
    
    auto device_it = std::find_if(m_devices.begin(), m_devices.end(),
                                  [&device](const Ptr<Rhi::IDevice>& system_device_ptr)
                                  { return std::addressof(device) == system_device_ptr.get(); });
    if (device_it == m_devices.end())
        return next_device_ptr;
    
    return device_it == m_devices.end() - 1 ? m_devices.front() : *(device_it + 1);
}

Ptr<Rhi::IDevice> System::GetSoftwareGpuDevice() const noexcept
{
    META_FUNCTION_TASK();
    auto sw_device_it = std::find_if(m_devices.begin(), m_devices.end(),
        [](const Ptr<Rhi::IDevice>& system_device_ptr)
        { return system_device_ptr && system_device_ptr->IsSoftwareAdapter(); });

    return sw_device_it != m_devices.end() ? *sw_device_it : Ptr<Rhi::IDevice>();
}

std::string System::ToString() const
{
    META_FUNCTION_TASK();
    std::stringstream ss;
    ss << "Available graphics devices:" << std::endl;
    for(const Ptr<Rhi::IDevice>& device_ptr : m_devices)
    {
        META_CHECK_NOT_NULL(device_ptr);
        ss << "  - " << device_ptr->ToString() << ";" << std::endl;
    }
    return ss.str();
}

} // namespace Methane::Graphics::Base