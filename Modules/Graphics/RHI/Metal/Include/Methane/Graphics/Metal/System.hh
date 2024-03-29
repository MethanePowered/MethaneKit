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

FILE: Methane/Graphics/Metal/System.hh
Metal implementation of the system interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/System.h>

#import <Metal/Metal.h>

namespace Methane::Graphics::Metal
{

class System final
    : public Base::System
{
public:
    ~System() override;
    
    void CheckForChanges() override {}
    const Ptrs<Rhi::IDevice>& UpdateGpuDevices(const Platform::AppEnvironment& app_env, const Rhi::DeviceCaps& required_device_caps) override;
    const Ptrs<Rhi::IDevice>& UpdateGpuDevices(const Rhi::DeviceCaps& required_device_caps) override;
    
private:
    void AddDevice(const id<MTLDevice>& mtl_device);
    const Ptr<Rhi::IDevice>& FindMetalDevice(const id<MTLDevice>& mtl_device) const;

#ifdef APPLE_MACOS
    void OnDeviceNotification(id<MTLDevice> mtl_device, MTLDeviceNotificationName device_notification);

    id<NSObject> m_device_observer = nil;
#endif
};

} // namespace Methane::Graphics::Metal
