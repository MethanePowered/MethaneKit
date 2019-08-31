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

FILE: Methane/Graphics/ContextBase.h
Base implementation of the context interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Context.h>
#include <Methane/Graphics/FpsCounter.h>
#include <Methane/Data/Provider.h>

#include "ObjectBase.h"
#include "DeviceBase.h"
#include "CommandQueueBase.h"
#include "RenderCommandListBase.h"
#include "RenderStateBase.h"
#include "RenderPassBase.h"
#include "ResourceManager.h"

#include <memory>
#include <vector>
#include <atomic>

namespace Methane
{
namespace Graphics
{

class ContextBase
    : public ObjectBase
    , public Context
{
public:
    ContextBase(const Data::Provider& data_provider, DeviceBase& device, const Settings& settings);

    // Context interface
    void                  CompleteInitialization() override;
    void                  WaitForGpu(WaitFor wait_for) override;
    void                  Resize(const FrameSize& frame_size) override;
    void                  Reset(Device& device) override;
    void                  Present() override;
    void                  AddCallback(Callback& callback) override;
    void                  RemoveCallback(Callback& callback) override;
    CommandQueue&         GetRenderCommandQueue() override;
    CommandQueue&         GetUploadCommandQueue() override;
    RenderCommandList&    GetUploadCommandList() override;
    Device&               GetDevice() override;
    const Data::Provider& GetDataProvider() const override      { return m_data_provider; }
    const Settings&       GetSettings() const override          { return m_settings; }
    uint32_t              GetFrameBufferIndex() const override  { return m_frame_buffer_index;  }
    const FpsCounter&     GetFpsCounter() const override        { return m_fps_counter; }
    bool                  SetVSyncEnabled(bool vsync_enabled) override;
    bool                  SetFrameBuffersCount(uint32_t frame_buffers_count) override;

    // ContextBase interface
    virtual void OnCommandQueueCompleted(CommandQueue& cmd_queue, uint32_t frame_index) = 0;

    // Object interface
    void SetName(const std::string& name) override;

    ResourceManager&  GetResourceManager() { return m_resource_manager; }

    DeviceBase& GetDeviceBase();
    const DeviceBase& GetDeviceBase() const;

protected:
    void UploadResources();
    void OnPresentComplete();
    
    virtual void Release();
    virtual void Initialize(Device& device, bool deferred_heap_allocation);

    using Callbacks = std::vector<Callback::Ref>;

    const Data::Provider&       m_data_provider;
    DeviceBase::Ptr             m_sp_device;
    Settings                    m_settings;
    ResourceManager::Settings   m_resource_manager_init_settings = { true };
    ResourceManager             m_resource_manager;
    Callbacks                   m_callbacks; // ORDER: Keep callbacks before resources for correct auto-delete
    CommandQueue::Ptr           m_sp_render_cmd_queue;
    CommandQueue::Ptr           m_sp_upload_cmd_queue;
    RenderCommandList::Ptr      m_sp_upload_cmd_list;
    std::atomic<uint32_t>       m_frame_buffer_index;
    FpsCounter                  m_fps_counter;
};

} // namespace Graphics
} // namespace Methane
