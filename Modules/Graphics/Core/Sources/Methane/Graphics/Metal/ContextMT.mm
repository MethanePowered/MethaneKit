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

#include <Methane/Graphics/Instrumentation.h>
#include <Methane/Platform/MacOS/Types.hh>

using namespace Methane::Graphics;

Context::Ptr Context::Create(const Platform::AppEnvironment& env, const Data::Provider& data_provider, Device& device, const Context::Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ContextMT>(env, data_provider, static_cast<DeviceBase&>(device), settings);
}

ContextMT::ContextMT(const Platform::AppEnvironment& env, const Data::Provider& data_provider, DeviceBase& device, const Context::Settings& settings)
    : ContextBase(data_provider, device, settings)
    , m_app_view([[AppViewMT alloc] initWithFrame: TypeConverterMT::CreateNSRect(m_settings.frame_size)
                                        appWindow: env.ns_app_delegate.window
                                           device: GetDeviceMT().GetNativeDevice()
                                      pixelFormat: TypeConverterMT::DataFormatToMetalPixelType(m_settings.color_format)
                                    drawableCount: m_settings.frame_buffers_count
                                     vsyncEnabled: Methane::MacOS::ConvertToNSType<bool, BOOL>(m_settings.vsync_enabled)
                            unsyncRefreshInterval: 1.0 / m_settings.unsync_max_fps])
    , m_dispatch_semaphore(dispatch_semaphore_create(m_settings.frame_buffers_count))
{
    ITT_FUNCTION_TASK();

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

    dispatch_semaphore_wait(m_dispatch_semaphore, DISPATCH_TIME_FOREVER);

    const bool switch_to_next_frame = (wait_for == WaitFor::FramePresented);
    ContextBase::WaitForGpu(wait_for);

    if (switch_to_next_frame)
    {
        m_frame_buffer_index = (m_frame_buffer_index + 1) % m_settings.frame_buffers_count;
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
    OnPresentComplete();
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

DeviceMT& ContextMT::GetDeviceMT()
{
    return static_cast<DeviceMT&>(GetDevice());
}
