/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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
#include "Instrumentation.h"

#include <sstream>

using namespace Methane;
using namespace Methane::Graphics;

std::string Device::Feature::ToString(Value feature) noexcept
{
    switch(feature)
    {
    case Value::Unknown:                    return "Unknown";
    case Value::All:                        return "All";
    case Value::BasicRendering:             return "Basic Rendering";
    case Value::TextureAndSamplerArrays:    return "Texture and Sampler Arrays";
    }
}

std::string Device::Feature::ToString(Mask features) noexcept
{
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

DeviceBase::DeviceBase(const std::string& name,
                       Feature::Mask      supported_features)
    : m_name(name)
    , m_supported_features(supported_features)
{
    ITT_FUNCTION_TASK();
}

void DeviceBase::Notify(Notification notification)
{
    if (m_notification_callback)
    {
        m_notification_callback(*this, notification);
    }
}

std::string DeviceBase::ToString() const noexcept
{
    return GetName() + " with features: " + Feature::ToString(m_supported_features);
}

Device::Ptr SystemBase::GetNextGpuDevice(const Device::Ptr& sp_device) const
{
    Device::Ptr sp_next_device;
    
    if (m_devices.empty())
        return sp_next_device;
    
    auto device_it = std::find_if(m_devices.begin(), m_devices.end(),
                                  [&sp_device](const Device::Ptr& sp_system_device)
                                  { return sp_device.get() == sp_system_device.get(); });
    if (device_it == m_devices.end())
        return sp_next_device;
    
    return device_it == m_devices.end() - 1 ? m_devices.front() : *(device_it + 1);
}

std::string SystemBase::ToString() const noexcept
{
    std::stringstream ss;
    ss << m_devices.size() << " system graphics device(s):" << std::endl;
    for(const Device::Ptr& sp_device : m_devices)
    {
        assert(sp_device);
        if (!sp_device) continue;
        ss << "  - " << sp_device->ToString() << ";" << std::endl;
    }
    return ss.str();
}
