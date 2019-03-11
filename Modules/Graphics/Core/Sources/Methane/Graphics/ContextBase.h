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
#include "CommandQueueBase.h"
#include "RenderCommandListBase.h"
#include "RenderStateBase.h"
#include "RenderPassBase.h"
#include "ResourceManager.h"

#include <memory>
#include <list>
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
    ContextBase(const Data::Provider& data_provider, const Settings& settings);
    virtual ~ContextBase() override = default;

    // Context interface
    virtual void                  CompleteInitialization() override;
    virtual void                  WaitForGpu(WaitFor wait_for) override;
    virtual void                  Resize(const FrameSize& frame_size) override;
    virtual CommandQueue&         GetRenderCommandQueue() override;
    virtual CommandQueue&         GetUploadCommandQueue() override;
    virtual RenderCommandList&    GetUploadCommandList() override;
    virtual const Data::Provider& GetDataProvider() const override      { return m_data_provider; }
    virtual const Settings&       GetSettings() const override          { return m_settings; }
    virtual uint32_t              GetFrameBufferIndex() const override  { return m_frame_buffer_index;  }
    virtual const FpsCounter&     GetFpsCounter() const override        { return m_fps_counter; }

    // ContextBase interface
    virtual void OnCommandQueueCompleted(CommandQueue& cmd_queue, uint32_t frame_index) = 0;

    ResourceManager&  GetResourceManager() { return m_resource_manager; }

protected:
    void UploadResources();
    void OnPresentComplete();

    const Data::Provider&  m_data_provider;
    Settings               m_settings;
    ResourceManager        m_resource_manager;
    CommandQueue::Ptr      m_sp_render_cmd_queue;
    CommandQueue::Ptr      m_sp_upload_cmd_queue;
    RenderCommandList::Ptr m_sp_upload_cmd_list;
    std::atomic<uint32_t>  m_frame_buffer_index;
    FpsCounter             m_fps_counter;
};

} // namespace Graphics
} // namespace Methane
