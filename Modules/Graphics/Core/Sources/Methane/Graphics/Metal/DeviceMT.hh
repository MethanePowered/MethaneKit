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

FILE: Methane/Graphics/Metal/DeviceMT.hh
Metal implementation of the device interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/DeviceBase.h>

#import <Metal/Metal.h>

namespace Methane::Graphics
{

class DeviceMT final : public DeviceBase
{
public:
    static Device::Feature::Mask GetSupportedFeatures(const id<MTLDevice>& mtl_device);
    
    DeviceMT(const id<MTLDevice>& mtl_device);
    ~DeviceMT() override;
    
    id<MTLDevice>& GetNativeDevice() { return m_mtl_device; }

private:
    id<MTLDevice> m_mtl_device;
};

class SystemMT final : public SystemBase
{
public:
    ~SystemMT() override;
    
    void           CheckForChanges() override {}
    const Ptrs<Device>& UpdateGpuDevices(Device::Feature::Mask supported_features) override;
    
private:
    void OnDeviceNotification(id<MTLDevice> mtl_device, MTLDeviceNotificationName device_notification);
    void NotifyDevice(const id<MTLDevice>& mtl_device, Device::Notification device_notification);
    void AddDevice(const id<MTLDevice>& mtl_device);
    
    const Ptr<Device>& FindMetalDevice(const id<MTLDevice>& mtl_device) const;
    
    id<NSObject> m_device_observer = nil;
};

} // namespace Methane::Graphics
