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

FILE: Methane/Graphics/RenderContext.h
Methane render context interface: represents graphics device and swap chain,
provides basic multi-frame rendering synchronization and frame presenting APIs.

******************************************************************************/

#pragma once

#include "Context.h"

#include <Methane/Platform/AppEnvironment.h>
#include <Methane/Platform/AppView.h>

#include <optional>

namespace Methane::Graphics
{

class FpsCounter;
struct RenderCommandList;

struct RenderContext : virtual Context
{
    struct Settings
    {
        FrameSize                   frame_size;
        PixelFormat                 color_format            = PixelFormat::BGRA8Unorm;
        PixelFormat                 depth_stencil_format    = PixelFormat::Unknown;
        std::optional<Color4f>      clear_color;
        std::optional<DepthStencil> clear_depth_stencil;
        uint32_t                    frame_buffers_count     = 3;
        bool                        vsync_enabled           = true;
        bool                        is_full_screen          = false;
        uint32_t                    unsync_max_fps          = 1000; // MacOS only
    };

    // Create RenderContext instance
    static Ptr<RenderContext> Create(const Platform::AppEnvironment& env, Device& device, const Settings& settings);

    // RenderContext interface
    virtual bool ReadyToRender() const = 0;
    virtual void Resize(const FrameSize& frame_size) = 0;
    virtual void Present() = 0;

    virtual Platform::AppView     GetAppView() const = 0;
    virtual CommandQueue&         GetRenderCommandQueue() = 0;
    virtual const Settings&       GetSettings() const = 0;
    virtual uint32_t              GetFrameBufferIndex() const = 0;
    virtual float                 GetContentScalingFactor() const = 0;
    virtual const FpsCounter&     GetFpsCounter() const = 0;

    virtual bool SetVSyncEnabled(bool vsync_enabled) = 0;
    virtual bool SetFrameBuffersCount(uint32_t frame_buffers_count) = 0;
    virtual bool SetFullScreen(bool is_full_screen) = 0;
};

} // namespace Methane::Graphics
