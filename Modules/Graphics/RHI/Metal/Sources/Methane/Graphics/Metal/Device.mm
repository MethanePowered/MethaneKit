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

FILE: Methane/Graphics/Metal/Device.mm
Metal implementation of the device interface.

******************************************************************************/

#include <Methane/Graphics/Metal/Device.hh>
#include <Methane/Graphics/Metal/RenderContext.hh>
#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Metal
{

Rhi::DeviceFeatureMask Device::GetSupportedFeatures(const id<MTLDevice>& mtl_device)
{
    META_FUNCTION_TASK();
    Rhi::DeviceFeatureMask supported_features;
    supported_features.SetBitOn(Rhi::DeviceFeature::PresentToWindow);
    supported_features.SetBitOn(Rhi::DeviceFeature::AnisotropicFiltering);
    supported_features.SetBit(Rhi::DeviceFeature::ImageCubeArray,
                              [mtl_device supportsFamily: MTLGPUFamilyCommon2] ||
                              [mtl_device supportsFamily: MTLGPUFamilyCommon3]);
    return supported_features;
}

Device::Device(const id<MTLDevice>& mtl_device, const Capabilities& capabilities)
    : Base::Device(MacOS::ConvertFromNsString(mtl_device.name), false, capabilities)
    , m_mtl_device(mtl_device)
{
    META_FUNCTION_TASK();
}

Ptr<Rhi::IRenderContext> Device::CreateRenderContext(const Platform::AppEnvironment& env, tf::Executor& parallel_executor, const Rhi::RenderContextSettings& settings)
{
    META_FUNCTION_TASK();
    const auto render_context_ptr = std::make_shared<RenderContext>(env, *this, parallel_executor, settings);
    render_context_ptr->Initialize(*this, true);
    return render_context_ptr;
}

} // namespace Methane::Graphics::Metal
