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

FILE: Methane/Graphics/ContextBase.cpp
Base implementation of the context interface.

******************************************************************************/

#include "ContextBase.h"
#include "Instrumentation.h"

#include <cassert>

using namespace Methane;
using namespace Methane::Graphics;

ContextBase::ContextBase(const Data::Provider& data_provider, DeviceBase& device, const Settings& settings)
    : m_data_provider(data_provider)
    , m_sp_device(device.GetPtr())
    , m_settings(settings)
    , m_resource_manager(*this)
    , m_frame_buffer_index(0)
{
    ITT_FUNCTION_TASK();
}

void ContextBase::CompleteInitialization()
{
    ITT_FUNCTION_TASK();
    m_resource_manager.CompleteInitialization();
    UploadResources();
}

void ContextBase::WaitForGpu(WaitFor)
{
    ITT_FUNCTION_TASK();
    m_resource_manager.GetReleasePool().ReleaseResources();
}

void ContextBase::Resize(const FrameSize& frame_size)
{
    ITT_FUNCTION_TASK();
    m_settings.frame_size = frame_size;
}

void ContextBase::Reset(Device& device)
{
    m_sp_device = static_cast<DeviceBase&>(device).GetPtr();

    for(const ICallback::Ref& callback_ref : m_callbacks)
    {
        callback_ref.get().OnContextReset(device);
    }
}

void ContextBase::ResetInternal(DeviceBase& device)
{
    m_sp_device = device.GetPtr();

    if (m_sp_upload_cmd_list)
    {
        static_cast<RenderCommandListBase&>(*m_sp_upload_cmd_list).OnContextReset(device);
    }
    if (m_sp_render_cmd_queue)
    {
        static_cast<CommandQueueBase&>(*m_sp_render_cmd_queue).OnContextReset(device);
    }
    if (m_sp_upload_cmd_queue)
    {
        static_cast<CommandQueueBase&>(*m_sp_upload_cmd_queue).OnContextReset(device);
    }
}

void ContextBase::AddCallback(ICallback& callback)
{
    m_callbacks.push_back(callback);
}

void ContextBase::RemoveCallback(ICallback& callback)
{
    const auto callback_it = std::find_if(m_callbacks.begin(), m_callbacks.end(),
                                          [&callback](const ICallback::Ref& callback_ref)
                                          { return std::addressof(callback_ref.get()) == std::addressof(callback); });
    assert(callback_it != m_callbacks.end());
    if (callback_it == m_callbacks.end())
        return;
    
    m_callbacks.erase(callback_it);
}

void ContextBase::OnPresentComplete()
{
    ITT_FUNCTION_TASK();
    m_fps_counter.OnFramePresented();
}

CommandQueue& ContextBase::GetRenderCommandQueue()
{
    ITT_FUNCTION_TASK();
    if (!m_sp_render_cmd_queue)
    {
        m_sp_render_cmd_queue = CommandQueue::Create(*this);
        m_sp_render_cmd_queue->SetName("Render Command Queue");
    }
    return *m_sp_render_cmd_queue;
}

CommandQueue& ContextBase::GetUploadCommandQueue()
{
    ITT_FUNCTION_TASK();
    if (!m_sp_upload_cmd_queue)
    {
        m_sp_upload_cmd_queue = CommandQueue::Create(*this);
        m_sp_upload_cmd_queue->SetName("Upload Command Queue");
    }
    return *m_sp_upload_cmd_queue;
}

RenderCommandList& ContextBase::GetUploadCommandList()
{
    ITT_FUNCTION_TASK();
    if (!m_sp_upload_cmd_list)
    {
        RenderPass::Ptr sp_empty_pass = RenderPass::Create(*this, RenderPass::Settings());
        m_sp_upload_cmd_list = RenderCommandList::Create(GetUploadCommandQueue(), *sp_empty_pass);
        m_sp_upload_cmd_list->SetName("Upload Command List");
    }
    return *m_sp_upload_cmd_list;
}

Device& ContextBase::GetDevice()
{
    ITT_FUNCTION_TASK();
    assert(!!m_sp_device);
    return *m_sp_device;
}

DeviceBase& ContextBase::GetDeviceBase()
{
    ITT_FUNCTION_TASK();
    return static_cast<DeviceBase&>(GetDevice());
}

const DeviceBase& ContextBase::GetDeviceBase() const
{
    ITT_FUNCTION_TASK();
    assert(!!m_sp_device);
    return static_cast<const DeviceBase&>(*m_sp_device);
}

bool ContextBase::SetVSyncEnabled(bool vsync_enabled)
{
    ITT_FUNCTION_TASK();
    if (m_settings.vsync_enabled == vsync_enabled)
        return false;

    m_settings.vsync_enabled = vsync_enabled;
    return true;
}

void ContextBase::UploadResources()
{
    ITT_FUNCTION_TASK();
    GetUploadCommandList().Commit(false);
    GetUploadCommandQueue().Execute({ GetUploadCommandList() });
    WaitForGpu(WaitFor::ResourcesUploaded);
}
