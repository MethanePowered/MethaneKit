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

FILE: Methane/Graphics/Metal/RenderContextMT.mm
Metal implementation of the render context interface.

******************************************************************************/

#include "RenderContextMT.hh"
#include "DeviceMT.hh"
#include "RenderStateMT.hh"
#include "RenderPassMT.hh"
#include "CommandQueueMT.hh"
#include "TypesMT.hh"

#include <Methane/Instrumentation.h>
#include <Methane/Platform/Utils.h>
#include <Methane/Platform/MacOS/Types.hh>

namespace Methane::Graphics
{

Ptr<RenderContext> RenderContext::Create(const Platform::AppEnvironment& env, Device& device, const RenderContext::Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<RenderContextMT>(env, static_cast<DeviceBase&>(device), settings);
}

RenderContextMT::RenderContextMT(const Platform::AppEnvironment& env, DeviceBase& device, const RenderContext::Settings& settings)
    : ContextMT<RenderContextBase>(device, settings)
    , m_app_view([[AppViewMT alloc] initWithFrame: TypeConverterMT::CreateNSRect(m_settings.frame_size)
                                        appWindow: env.ns_app_delegate.window
                                           device: GetDeviceMT().GetNativeDevice()
                                      pixelFormat: TypeConverterMT::DataFormatToMetalPixelType(m_settings.color_format)
                                    drawableCount: m_settings.frame_buffers_count
                                     vsyncEnabled: Methane::MacOS::ConvertToNSType<bool, BOOL>(m_settings.vsync_enabled)
                            unsyncRefreshInterval: 1.0 / m_settings.unsync_max_fps])
    , m_frame_capture_scope([[MTLCaptureManager sharedCaptureManager] newCaptureScopeWithDevice:GetDeviceMT().GetNativeDevice()])
{
    ITT_FUNCTION_TASK();
    
    m_frame_capture_scope.label = Methane::MacOS::ConvertToNSType<std::string, NSString*>(device.GetName() + " Capture Scope");
    [MTLCaptureManager sharedCaptureManager].defaultCaptureScope = m_frame_capture_scope;

    // bind metal context with application delegate
    m_app_view.delegate = env.ns_app_delegate;
    env.ns_app_delegate.view = m_app_view;

    // Start redrawing main view
    m_app_view.redrawing = YES;
}

RenderContextMT::~RenderContextMT()
{
    ITT_FUNCTION_TASK();

    [m_app_view release];
}

void RenderContextMT::Release()
{
    ITT_FUNCTION_TASK();
    
    m_app_view.redrawing = NO;

    ContextMT<RenderContextBase>::Release();
}

void RenderContextMT::Initialize(DeviceBase& device, bool deferred_heap_allocation)
{
    ITT_FUNCTION_TASK();

    ContextMT<RenderContextBase>::Initialize(device, deferred_heap_allocation);
    
    m_app_view.redrawing = YES;
}

bool RenderContextMT::ReadyToRender() const
{
    ITT_FUNCTION_TASK();
    return m_app_view.currentDrawable != nil;
}

void RenderContextMT::WaitForGpu(WaitFor wait_for)
{
    ITT_FUNCTION_TASK();

    ContextMT<RenderContextBase>::WaitForGpu(wait_for);

    if (wait_for == WaitFor::FramePresented)
    {
        m_frame_buffer_index = (m_frame_buffer_index + 1) % m_settings.frame_buffers_count;
        [m_frame_capture_scope beginScope];
    }
}

void RenderContextMT::Resize(const FrameSize& frame_size)
{
    ITT_FUNCTION_TASK();
    ContextMT<RenderContextBase>::Resize(frame_size);
}

void RenderContextMT::Present()
{
    ITT_FUNCTION_TASK();
    ContextMT<RenderContextBase>::Present();
    
    [m_frame_capture_scope endScope];

    ContextMT<RenderContextBase>::OnCpuPresentComplete();
}

bool RenderContextMT::SetVSyncEnabled(bool vsync_enabled)
{
    ITT_FUNCTION_TASK();
    if (ContextMT<RenderContextBase>::SetVSyncEnabled(vsync_enabled))
    {
        m_app_view.vsyncEnabled = vsync_enabled ? YES : NO;
        return true;
    }
    return false;
}

bool RenderContextMT::SetFrameBuffersCount(uint32_t frame_buffers_count)
{
    ITT_FUNCTION_TASK();
    frame_buffers_count = std::min(std::max(2u, frame_buffers_count), 3u); // Metal supports only 2 or 3 drawable buffers
    if (ContextMT<RenderContextBase>::SetFrameBuffersCount(frame_buffers_count))
    {
        m_app_view.drawableCount = frame_buffers_count;
        return true;
    }
    return false;
}

float RenderContextMT::GetContentScalingFactor() const
{
    ITT_FUNCTION_TASK();
    return m_app_view.appWindow.backingScaleFactor;
}
    
CommandQueueMT& RenderContextMT::GetRenderCommandQueueMT()
{
    ITT_FUNCTION_TASK();
    return static_cast<CommandQueueMT&>(ContextMT<RenderContextBase>::GetRenderCommandQueue());
}

} // namespace Methane::Graphics
