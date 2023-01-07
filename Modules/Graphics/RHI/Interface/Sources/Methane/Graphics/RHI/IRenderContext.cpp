/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/IRenderContext.cpp
Methane render context interface: represents graphics device and swap chain,
provides basic multi-frame rendering synchronization and frame presenting APIs.

******************************************************************************/

#include <Methane/Graphics/RHI/IRenderContext.h>
#include <Methane/Graphics/RHI/IDevice.h>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

RenderContextSettings& RenderContextSettings::SetFrameSize(FrameSize&& new_frame_size) noexcept
{
    META_FUNCTION_TASK();
    frame_size = std::move(new_frame_size);
    return *this;
}

RenderContextSettings& RenderContextSettings::SetColorFormat(PixelFormat new_color_format) noexcept
{
    META_FUNCTION_TASK();
    color_format = new_color_format;
    return *this;
}

RenderContextSettings& RenderContextSettings::SetDepthStencilFormat(PixelFormat new_ds_format) noexcept
{
    META_FUNCTION_TASK();
    depth_stencil_format = new_ds_format;
    return *this;
}

RenderContextSettings& RenderContextSettings::SetClearColor(Opt<Color4F>&& new_clear_color) noexcept
{
    META_FUNCTION_TASK();
    clear_color = std::move(new_clear_color);
    return *this;
}

RenderContextSettings& RenderContextSettings::SetClearDepthStencil(Opt<DepthStencilValues>&& new_clear_ds) noexcept
{
    META_FUNCTION_TASK();
    clear_depth_stencil = std::move(new_clear_ds);
    return *this;
}

RenderContextSettings& RenderContextSettings::SetFrameBuffersCount(uint32_t new_fb_count) noexcept
{
    META_FUNCTION_TASK();
    frame_buffers_count = new_fb_count;
    return *this;
}

RenderContextSettings& RenderContextSettings::SetVSyncEnabled(bool new_vsync_enabled) noexcept
{
    META_FUNCTION_TASK();
    vsync_enabled = new_vsync_enabled;
    return *this;
}

RenderContextSettings& RenderContextSettings::SetFullscreen(bool new_full_screen) noexcept
{
    META_FUNCTION_TASK();
    is_full_screen = new_full_screen;
    return *this;
}

RenderContextSettings& RenderContextSettings::SetOptionMask(ContextOptionMask new_options_mask) noexcept
{
    META_FUNCTION_TASK();
    options_mask = new_options_mask;
    return *this;
}

RenderContextSettings& RenderContextSettings::SetUnsyncMaxFps(uint32_t new_unsync_max_fps) noexcept
{
    META_FUNCTION_TASK();
    unsync_max_fps = new_unsync_max_fps;
    return *this;
}

Ptr<IRenderContext> IRenderContext::Create(const Platform::AppEnvironment& env, IDevice& device, tf::Executor& parallel_executor, const Settings& settings)
{
    META_FUNCTION_TASK();
    return device.CreateRenderContext(env, parallel_executor, settings);
}

} // namespace Methane::Graphics::Rhi
