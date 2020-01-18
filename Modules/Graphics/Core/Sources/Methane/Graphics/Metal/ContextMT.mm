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

FILE: Methane/Graphics/Metal/ContextMT.mm
Metal implementation of the context interface.

******************************************************************************/

#include "ContextMT.hh"
#include "DeviceMT.hh"
#include "RenderStateMT.hh"
#include "RenderPassMT.hh"
#include "CommandQueueMT.hh"
#include "TypesMT.hh"

#include <Methane/Data/Instrumentation.h>
#include <Methane/Platform/Utils.h>
#include <Methane/Platform/MacOS/Types.hh>

namespace Methane::Graphics
{

Ptr<Context> Context::Create(const Platform::AppEnvironment& env, Device& device, const Context::Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ContextMT>(env, static_cast<DeviceBase&>(device), settings);
}

ContextMT::ContextMT(const Platform::AppEnvironment& env, DeviceBase& device, const Context::Settings& settings)
    : ContextBase(device, settings)
    , m_app_view([[AppViewMT alloc] initWithFrame: TypeConverterMT::CreateNSRect(m_settings.frame_size)
                                        appWindow: env.ns_app_delegate.window
                                           device: GetDeviceMT().GetNativeDevice()
                                      pixelFormat: TypeConverterMT::DataFormatToMetalPixelType(m_settings.color_format)
                                    drawableCount: m_settings.frame_buffers_count
                                     vsyncEnabled: Methane::MacOS::ConvertToNSType<bool, BOOL>(m_settings.vsync_enabled)
                            unsyncRefreshInterval: 1.0 / m_settings.unsync_max_fps])
    , m_dispatch_semaphore(dispatch_semaphore_create(m_settings.frame_buffers_count))
    , m_frame_capture_scope([[MTLCaptureManager sharedCaptureManager] newCaptureScopeWithDevice:GetDeviceMT().GetNativeDevice()])
{
    ITT_FUNCTION_TASK();
    
    m_frame_capture_scope.label = Methane::MacOS::ConvertToNSType<std::string, NSString*>(device.GetName() + " Capture Scope");
    [MTLCaptureManager sharedCaptureManager].defaultCaptureScope = m_frame_capture_scope;

    // bind metal context with application delegate
    m_app_view.delegate = env.ns_app_delegate;
    env.ns_app_delegate.view = m_app_view;

    m_resource_manager.Initialize({ true });

    // Start redrawing main view
    m_app_view.redrawing = YES;
}

ContextMT::~ContextMT()
{
    ITT_FUNCTION_TASK();

    dispatch_release(m_dispatch_semaphore);

    [m_app_view release];
}

void ContextMT::Release()
{
    ITT_FUNCTION_TASK();
    
    m_app_view.redrawing = NO;
    
    // FIXME: semaphore release causes a crash
    // https://stackoverflow.com/questions/8287621/why-does-this-code-cause-exc-bad-instruction
    //dispatch_release(m_dispatch_semaphore);
    
    ContextBase::Release();
}

void ContextMT::Initialize(Device& device, bool deferred_heap_allocation)
{
    ITT_FUNCTION_TASK();
    
    m_dispatch_semaphore = dispatch_semaphore_create(m_settings.frame_buffers_count);
    
    ContextBase::Initialize(device, deferred_heap_allocation);
    
    m_app_view.redrawing = YES;
}

bool ContextMT::ReadyToRender() const
{
    ITT_FUNCTION_TASK();
    return m_app_view.currentDrawable != nil;
}

void ContextMT::OnCommandQueueCompleted(CommandQueue& /*cmd_queue*/, uint32_t /*frame_index*/)
{
    ITT_FUNCTION_TASK();

    dispatch_semaphore_signal(m_dispatch_semaphore);
}

void ContextMT::WaitForGpu(WaitFor wait_for)
{
    ITT_FUNCTION_TASK();
    
    ContextBase::WaitForGpu(wait_for);
    
    dispatch_semaphore_wait(m_dispatch_semaphore, DISPATCH_TIME_FOREVER);
    
    ContextBase::OnGpuWaitComplete(wait_for);

    if (wait_for == WaitFor::FramePresented)
    {
        m_frame_buffer_index = (m_frame_buffer_index + 1) % m_settings.frame_buffers_count;
        [m_frame_capture_scope beginScope];
    }
}

void ContextMT::Resize(const FrameSize& frame_size)
{
    ITT_FUNCTION_TASK();
    ContextBase::Resize(frame_size);
}

void ContextMT::Present()
{
    ITT_FUNCTION_TASK();
    ContextBase::Present();
    
    [m_frame_capture_scope endScope];
    
    OnCpuPresentComplete();
}

bool ContextMT::SetVSyncEnabled(bool vsync_enabled)
{
    ITT_FUNCTION_TASK();
    if (ContextBase::SetVSyncEnabled(vsync_enabled))
    {
        m_app_view.vsyncEnabled = vsync_enabled ? YES : NO;
        return true;
    }
    return false;
}

bool ContextMT::SetFrameBuffersCount(uint32_t frame_buffers_count)
{
    ITT_FUNCTION_TASK();
    frame_buffers_count = std::min(std::max(2u, frame_buffers_count), 3u); // Metal supports only 2 or 3 drawable buffers
    if (ContextBase::SetFrameBuffersCount(frame_buffers_count))
    {
        m_app_view.drawableCount = frame_buffers_count;
        return true;
    }
    return false;
}

float ContextMT::GetContentScalingFactor() const
{
    ITT_FUNCTION_TASK();
    return m_app_view.appWindow.backingScaleFactor;
}

DeviceMT& ContextMT::GetDeviceMT()
{
    ITT_FUNCTION_TASK();
    return static_cast<DeviceMT&>(GetDevice());
}
    
CommandQueueMT& ContextMT::GetRenderCommandQueueMT()
{
    ITT_FUNCTION_TASK();
    return static_cast<CommandQueueMT&>(ContextBase::GetRenderCommandQueue());
}

const Ptr<ContextMT::LibraryMT>& ContextMT::GetLibraryMT(const std::string& library_name)
{
    ITT_FUNCTION_TASK();
    const auto library_by_name_it = m_library_by_name.find(library_name);
    if (library_by_name_it != m_library_by_name.end())
        return library_by_name_it->second;

    return m_library_by_name.emplace(library_name, std::make_shared<LibraryMT>(*this, library_name)).first->second;
}

NSString* ContextMT::LibraryMT::GetFullPath(const std::string& library_name)
{
    return MacOS::ConvertToNSType<std::string, NSString*>(Platform::GetResourceDir() + "/" + library_name + ".metallib");
}

ContextMT::LibraryMT::LibraryMT(ContextMT& metal_context, const std::string& library_name)
        : m_mtl_library(library_name.empty()
                        ? [metal_context.GetDeviceMT().GetNativeDevice() newDefaultLibrary]
                        : [metal_context.GetDeviceMT().GetNativeDevice() newLibraryWithFile:GetFullPath(library_name) error:&m_ns_error])
{
    ITT_FUNCTION_TASK();
    if (!m_mtl_library)
    {
        const std::string error_msg = MacOS::ConvertFromNSType<NSString, std::string>([m_ns_error localizedDescription]);
        throw std::runtime_error("Failed to create " + (library_name.empty() ? std::string("default") : library_name) + " Metal library: " + error_msg);
    }
}

ContextMT::LibraryMT::~LibraryMT()
{
    ITT_FUNCTION_TASK();
    [m_mtl_library release];
}

} // namespace Methane::Graphics
