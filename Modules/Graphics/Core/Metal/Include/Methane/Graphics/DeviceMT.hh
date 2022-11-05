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

FILE: Methane/Graphics/Metal/DeviceMT.hh
Metal implementation of the device interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/Device.h>

#import <Metal/Metal.h>

namespace Methane::Graphics
{

class DeviceMT final : public Base::Device
{
public:
    static DeviceFeatures GetSupportedFeatures(const id<MTLDevice>& mtl_device);
    
    DeviceMT(const id<MTLDevice>& mtl_device, const Capabilities& capabilities);

    const id<MTLDevice>& GetNativeDevice() const { return m_mtl_device; }

private:
    id<MTLDevice> m_mtl_device;
};

class SystemMT final : public Base::System
{
public:
    ~SystemMT() override;
    
    void                 CheckForChanges() override {}
    const Ptrs<IDevice>& UpdateGpuDevices(const Platform::AppEnvironment& app_env, const DeviceCaps& required_device_caps) override;
    const Ptrs<IDevice>& UpdateGpuDevices(const DeviceCaps& required_device_caps) override;
    
private:
    void AddDevice(const id<MTLDevice>& mtl_device);
    const Ptr<IDevice>& FindMetalDevice(const id<MTLDevice>& mtl_device) const;

#ifdef APPLE_MACOS
    void OnDeviceNotification(id<MTLDevice> mtl_device, MTLDeviceNotificationName device_notification);

    id<NSObject> m_device_observer = nil;
#endif
};

} // namespace Methane::Graphics
