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

FILE: Methane/Graphics/DeviceBase.h
Base implementation of the device interface.

******************************************************************************/

#pragma once

#include "ObjectBase.h"

#include <Methane/Graphics/Device.h>
#include <Methane/Data/Emitter.hpp>

namespace Methane::Graphics
{

class DeviceBase
    : public Device
    , public ObjectBase
    , public Data::Emitter<IDeviceCallback>
{
public:
    DeviceBase(const std::string& adapter_name, bool is_software_adapter, const Capabilities& capabilities);

    // Device interface
    const std::string&  GetAdapterName() const noexcept override    { return m_adapter_name; }
    bool                IsSoftwareAdapter() const noexcept override { return m_is_software_adapter; }
    const Capabilities& GetCapabilities() const noexcept override   { return m_capabilities; }
    std::string         ToString() const override;
    
protected:
    friend class SystemBase;

    void OnRemovalRequested();
    void OnRemoved();

private:
    const std::string m_adapter_name;
    const bool        m_is_software_adapter;
    Capabilities      m_capabilities;
};

class SystemBase : public System
{
public:
    const Ptrs<Device>&         GetGpuDevices() const noexcept override          { return m_devices; }
    const Device::Capabilities& GetDeviceCapabilities() const noexcept override  { return m_device_caps; }
    Ptr<Device>                 GetNextGpuDevice(const Device& device) const noexcept override;
    Ptr<Device>                 GetSoftwareGpuDevice() const noexcept override;
    std::string                 ToString() const override;

protected:
    void SetDeviceCapabilities(const Device::Capabilities& device_caps) { m_device_caps = device_caps; }
    void ClearDevices() { m_devices.clear(); }
    void AddDevice(const Ptr<Device>& device_ptr) { m_devices.emplace_back(device_ptr); }
    void RequestRemoveDevice(Device& device) const;
    void RemoveDevice(Device& device);

private:
    Device::Capabilities m_device_caps;
    Ptrs<Device>         m_devices;
};

} // namespace Methane::Graphics
