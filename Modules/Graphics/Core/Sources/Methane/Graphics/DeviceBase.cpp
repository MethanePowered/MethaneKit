/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
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

#include <sstream>
#include <cassert>

namespace Methane::Graphics
{

std::string Device::Feature::ToString(Value feature) noexcept
{
    ITT_FUNCTION_TASK();
    switch(feature)
    {
    case Value::Unknown:                    return "Unknown";
    case Value::All:                        return "All";
    case Value::BasicRendering:             return "Basic Rendering";
    case Value::TextureAndSamplerArrays:    return "Texture and Sampler Arrays";
    }
    return "";
}

std::string Device::Feature::ToString(Mask features) noexcept
{
    ITT_FUNCTION_TASK();
    std::stringstream ss;
    bool is_first_feature = true;
    for(Value value : values)
    {
        if (!(features & value))
            continue;
        
        if (!is_first_feature)
            ss << ", ";
        
        ss << ToString(value);
        is_first_feature = false;
    }
    return ss.str();
}

DeviceBase::DeviceBase(const std::string& adapter_name, bool is_software_adapter, Feature::Mask supported_features)
    : m_adapter_name(adapter_name)
    , m_is_software_adapter(is_software_adapter)
    , m_supported_features(supported_features)
{
    ITT_FUNCTION_TASK();
}

void DeviceBase::Notify(Notification notification)
{
    ITT_FUNCTION_TASK();
    if (m_notification_callback)
    {
        m_notification_callback(*this, notification);
    }
}

std::string DeviceBase::ToString() const noexcept
{
    ITT_FUNCTION_TASK();
    std::stringstream ss;
    ss << "GPU \"" << GetAdapterName() << "\" with features: " + Feature::ToString(m_supported_features);
    return ss.str();
}

Ptr<Device> SystemBase::GetNextGpuDevice(const Device& device) const
{
    ITT_FUNCTION_TASK();
    Ptr<Device> sp_next_device;
    
    if (m_devices.empty())
        return sp_next_device;
    
    auto device_it = std::find_if(m_devices.begin(), m_devices.end(),
                                  [&device](const Ptr<Device>& sp_system_device)
                                  { return std::addressof(device) == sp_system_device.get(); });
    if (device_it == m_devices.end())
        return sp_next_device;
    
    return device_it == m_devices.end() - 1 ? m_devices.front() : *(device_it + 1);
}

Ptr<Device> SystemBase::GetSoftwareGpuDevice() const
{
    ITT_FUNCTION_TASK();
    auto sw_device_it = std::find_if(m_devices.begin(), m_devices.end(),
        [](const Ptr<Device>& sp_system_device)
        { return sp_system_device && sp_system_device->IsSoftwareAdapter(); });

    return sw_device_it != m_devices.end() ? *sw_device_it : Ptr<Device>();
}

std::string SystemBase::ToString() const noexcept
{
    ITT_FUNCTION_TASK();
    std::stringstream ss;
    ss << m_devices.size() << " system graphics device"
       << (m_devices.size() > 1 ? "s:" : ":") << std::endl;
    for(const Ptr<Device>& sp_device : m_devices)
    {
        assert(sp_device);
        if (!sp_device) continue;
        ss << "  - " << sp_device->ToString() << ";" << std::endl;
    }
    return ss.str();
}

} // namespace Methane::Graphics