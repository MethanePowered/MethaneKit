/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Context.h
Methane context interface: represents graphics device and swap chain,
provides basic multi-frame rendering synchronization and frame presenting APIs.

******************************************************************************/

#pragma once

#include "Object.h"
#include "Types.h"

#include <Methane/Data/Provider.h>
#include <Methane/Platform/AppEnvironment.h>
#include <Methane/Platform/AppView.h>

#include <memory>

namespace Methane
{
namespace Graphics
{

class FpsCounter;
struct Device;
struct CommandQueue;
struct RenderCommandList;

struct Context : virtual Object
{
    using Ptr = std::shared_ptr<Context>;

    struct Settings
    {
        FrameSize   frame_size;
        PixelFormat color_format            = PixelFormat::BGRA8Unorm;
        PixelFormat depth_stencil_format    = PixelFormat::Unknown;
        Color       clear_color;
        Depth       clear_depth             = 1.f;
        Stencil     clear_stencil           = 0;
        uint32_t    frame_buffers_count     = 3;
        bool        vsync_enabled           = true;
        uint32_t    unsync_max_fps          = 1000; // MacOS only
    };

    enum class WaitFor
    {
        RenderComplete,
        FramePresented,
        ResourcesUploaded
    };
    
    struct Callback
    {
        using Ref = std::reference_wrapper<Callback>;
        
        virtual void OnContextReleased() = 0;
        virtual void OnContextInitialized() = 0;
        
        virtual ~Callback() = default;
    };

    // Create Context instance
    static Ptr Create(const Platform::AppEnvironment& env, const Data::Provider& data_provider, Device& device, const Settings& settings);

    // Context interface
    virtual void CompleteInitialization() = 0;
    virtual bool ReadyToRender() const = 0;
    virtual void WaitForGpu(WaitFor wait_for) = 0;
    virtual void Resize(const FrameSize& frame_size) = 0;
    virtual void Reset(Device& device) = 0;
    virtual void Present() = 0;
    
    virtual void AddCallback(Callback& callback) = 0;
    virtual void RemoveCallback(Callback& callback) = 0;

    virtual Platform::AppView     GetAppView() const = 0;
    virtual const Data::Provider& GetDataProvider() const = 0;
    virtual Device&               GetDevice() = 0;
    virtual CommandQueue&         GetRenderCommandQueue() = 0;
    virtual CommandQueue&         GetUploadCommandQueue() = 0;
    virtual RenderCommandList&    GetUploadCommandList() = 0;
    virtual const Settings&       GetSettings() const = 0;
    virtual uint32_t              GetFrameBufferIndex() const = 0;
    virtual const FpsCounter&     GetFpsCounter() const = 0;

    virtual bool SetVSyncEnabled(bool vsync_enabled) = 0;
    virtual bool SetFrameBuffersCount(uint32_t frame_buffers_count) = 0;
};

} // namespace Graphics
} // namespace Methane
