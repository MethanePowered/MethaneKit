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

FILE: Methane/Graphics/Base/Device.h
Base implementation of the device interface.

******************************************************************************/

#pragma once

#include "Object.h"

#include <Methane/Graphics/RHI/IDevice.h>
#include <Methane/Data/Emitter.hpp>

namespace Methane::Graphics::Base
{

class System;

class Device
    : public Rhi::IDevice
    , public Object
    , public Data::Emitter<Rhi::IDeviceCallback>
{
public:
    Device(const std::string& adapter_name, bool is_software_adapter, const Capabilities& capabilities);

    // IDevice interface
    const std::string&  GetAdapterName() const noexcept override    { return m_adapter_name; }
    bool                IsSoftwareAdapter() const noexcept override { return m_is_software_adapter; }
    const Capabilities& GetCapabilities() const noexcept override   { return m_capabilities; }
    std::string         ToString() const override;
    
protected:
    friend class System;

    void OnRemovalRequested();
    void OnRemoved();

private:
    // ISystem should be released only after all its devices, so devices hold it's shared pointer
    const Ptr<System> m_system_ptr;
    const std::string     m_adapter_name;
    const bool            m_is_software_adapter;
    Capabilities          m_capabilities;
};

class System
    : public Rhi::ISystem
    , public std::enable_shared_from_this<System>
{
public:
    // ISystem interface
    const Ptrs<Rhi::IDevice>& GetGpuDevices() const noexcept override          { return m_devices; }
    const Rhi::DeviceCaps&    GetDeviceCapabilities() const noexcept override  { return m_device_caps; }
    Ptr<Rhi::IDevice>         GetNextGpuDevice(const Rhi::IDevice& device) const noexcept override;
    Ptr<Rhi::IDevice>         GetSoftwareGpuDevice() const noexcept override;
    std::string               ToString() const override;

    Ptr<System> GetPtr() { return shared_from_this(); }

protected:
    void SetDeviceCapabilities(const Rhi::DeviceCaps& device_caps) { m_device_caps = device_caps; }
    void ClearDevices() { m_devices.clear(); }
    void AddDevice(const Ptr<Rhi::IDevice>& device_ptr) { m_devices.emplace_back(device_ptr); }
    void RequestRemoveDevice(Rhi::IDevice& device) const;
    void RemoveDevice(Rhi::IDevice& device);

private:
    Rhi::DeviceCaps    m_device_caps;
    Ptrs<Rhi::IDevice> m_devices;
};

} // namespace Methane::Graphics::Base
