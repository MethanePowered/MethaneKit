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

FILE: Methane/Graphics/Base/System.h
Base implementation of the system interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RHI/ISystem.h>

namespace Methane::Graphics::Base
{

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
