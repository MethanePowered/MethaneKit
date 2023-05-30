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

FILE: Methane/Graphics/Metal/Device.hh
Metal implementation of the device interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/Device.h>

#import <Metal/Metal.h>

namespace Methane::Graphics::Metal
{

class Device final
    : public Base::Device
{
public:
    static Rhi::DeviceFeatureMask GetSupportedFeatures(const id<MTLDevice>& mtl_device);
    
    Device(const id<MTLDevice>& mtl_device, const Capabilities& capabilities);

    // IDevice interface
    [[nodiscard]] Ptr<Rhi::IRenderContext>  CreateRenderContext(const Platform::AppEnvironment& env, tf::Executor& parallel_executor, const Rhi::RenderContextSettings& settings) override;
    [[nodiscard]] Ptr<Rhi::IComputeContext> CreateComputeContext(tf::Executor& parallel_executor, const Rhi::ComputeContextSettings& settings) override;

    const id<MTLDevice>& GetNativeDevice() const { return m_mtl_device; }

private:
    id<MTLDevice> m_mtl_device;
};

} // namespace Methane::Graphics::Metal
