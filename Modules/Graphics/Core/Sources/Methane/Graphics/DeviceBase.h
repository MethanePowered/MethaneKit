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

FILE: Methane/Graphics/DeviceBase.h
Base implementation of the device interface.

******************************************************************************/

#pragma once

#include "ObjectBase.h"

#include <Methane/Graphics/Device.h>

namespace Methane::Graphics
{

class DeviceBase
    : public Device
    , public ObjectBase
    , public std::enable_shared_from_this<DeviceBase>
{
public:
    DeviceBase(const std::string& adapter_name, bool is_software_adapter, Feature::Mask supported_features);

    // Device interface
    const std::string&  GetAdapterName() const noexcept override                               { return m_adapter_name; }
    bool                IsSoftwareAdapter() const noexcept override                            { return m_is_software_adapter; }
    Feature::Mask       GetSupportedFeatures() const noexcept override                         { return m_supported_features; }
    void                SetNotificationCallback(const NotificationCallback& callback) override { m_notification_callback = callback; }
    void                Notify(Notification notification) override;
    std::string         ToString() const noexcept override;

    Ptr<DeviceBase> GetPtr() { return shared_from_this(); }

private:
    const std::string    m_adapter_name;
    const bool           m_is_software_adapter;
    const Feature::Mask  m_supported_features;
    NotificationCallback m_notification_callback;
};

class SystemBase : public System
{
public:
    const Ptrs<Device>&   GetGpuDevices() const override            { return m_devices; }
    Device::Feature::Mask GetGpuSupportedFeatures() const override  { return m_supported_features; }
    Ptr<Device>           GetNextGpuDevice(const Device& device) const override;
    Ptr<Device>           GetSoftwareGpuDevice() const override;
    std::string           ToString() const noexcept override;

protected:
    void SetGpuSupportedFeatures(Device::Feature::Mask supported_features) { m_supported_features = supported_features; }
    void ClearDevices()                     { m_devices.clear(); }
    void AddDevice(Ptr<Device>&& sp_device) { m_devices.emplace_back(std::move(sp_device)); }

private:
    Device::Feature::Mask m_supported_features = Device::Feature::Value::All;
    Ptrs<Device>          m_devices;
};

} // namespace Methane::Graphics
