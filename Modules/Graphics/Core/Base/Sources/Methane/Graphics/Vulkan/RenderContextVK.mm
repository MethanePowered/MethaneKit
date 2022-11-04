/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/RenderContextVK.mm
Vulkan implementation of the render context interface specific to MacOS.

******************************************************************************/

#include "RenderContextVK.h"
#include "PlatformVK.h"

#include <Methane/Graphics/Metal/RenderContextAppViewMT.hh>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

RenderContextVK::RenderContextVK(const Platform::AppEnvironment& app_env, DeviceVK& device, tf::Executor& parallel_executor, const RenderContextSettings& settings)
    : ContextVK<RenderContextBase>(device, parallel_executor, settings)
    , m_vk_device(device.GetNativeDevice())
    , m_metal_view(CreateRenderContextAppView(app_env, settings))
    , m_vk_unique_surface(PlatformVK::CreateVulkanSurfaceForWindow(static_cast<SystemVK&>(System::Get()).GetNativeInstance(), app_env))
{
    META_FUNCTION_TASK();

    // Start redrawing metal view
    m_metal_view.redrawing = YES;
}

}
