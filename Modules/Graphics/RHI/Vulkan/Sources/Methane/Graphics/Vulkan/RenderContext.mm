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

FILE: Methane/Graphics/Vulkan/RenderContext.mm
Vulkan implementation of the render context interface specific to MacOS.

******************************************************************************/

#include <Methane/Graphics/Vulkan/RenderContext.h>
#include <Methane/Graphics/Vulkan/System.h>
#include <Methane/Graphics/Vulkan/Platform.h>
#include <Methane/Graphics/Metal/RenderContextAppView.hh>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Vulkan
{

RenderContext::RenderContext(const Methane::Platform::AppEnvironment& app_env, Device& device, tf::Executor& parallel_executor, const Rhi::RenderContextSettings& settings)
    : Context<Base::RenderContext>(device, parallel_executor, settings)
    , m_metal_view(Metal::CreateRenderContextAppView(app_env, settings))
    , m_app_env(app_env)
    , m_vk_device(device.GetNativeDevice())
    , m_vk_unique_surface(Platform::CreateVulkanSurfaceForWindow(static_cast<System&>(Rhi::ISystem::Get()).GetNativeInstance(), app_env))
{
    META_FUNCTION_TASK();

    // Start redrawing metal view
    m_metal_view.redrawing = YES;
}

bool RenderContext::SetVSyncEnabled(bool vsync_enabled)
{
    META_FUNCTION_TASK();
    if (Base::RenderContext::SetVSyncEnabled(vsync_enabled))
    {
        ResetNativeSwapchain();
        m_metal_view.vsyncEnabled = vsync_enabled;
        return true;
    }
    return false;
}

} // namespace Methane::Graphics::Vulkan
