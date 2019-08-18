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

FILE: Methane/Graphics/DeviceBase.h
Base implementation of the device interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Device.h>

namespace Methane
{
namespace Graphics
{

class DeviceBase
    : public Device
    , public std::enable_shared_from_this<DeviceBase>
{
public:
    using Ptr = std::shared_ptr<DeviceBase>;
    
    DeviceBase(const std::string& name,
               Feature::Mask supported_features);

    // Device interface
    const std::string&  GetName() const noexcept override                                      { return m_name; }
    Feature::Mask       GetSupportedFeatures() const noexcept override                         { return m_supported_features; }
    void                SetNotificationCallback(const NotificationCallback& callback) override { m_notification_callback = callback; }
    void                Notify(Notification notification) override;
    std::string         ToString() const noexcept override;
    
    Ptr GetPtr() { return shared_from_this(); }

protected:
    const std::string    m_name;
    const Feature::Mask  m_supported_features;
    NotificationCallback m_notification_callback;
};
    
class SystemBase : public System
{
public:
    const Devices&        GetGpuDevices() const override            { return m_devices; }
    Device::Feature::Mask GetGpuSupportedFeatures() const override  { return m_supported_features; }
    Device::Ptr           GetNextGpuDevice(const Device& device) const override;
    std::string           ToString() const noexcept override;
    
protected:
    Device::Feature::Mask m_supported_features = Device::Feature::Value::All;
    Devices               m_devices;
};

} // namespace Graphics
} // namespace Methane
