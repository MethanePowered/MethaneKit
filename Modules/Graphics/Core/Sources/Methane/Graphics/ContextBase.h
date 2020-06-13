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

FILE: Methane/Graphics/ContextBase.h
Base implementation of the context interface.

******************************************************************************/

#pragma once

#include "ObjectBase.h"
#include "ResourceManager.h"

#include <Methane/Graphics/Fence.h>
#include <Methane/Graphics/Context.h>
#include <Methane/Graphics/Native/ContextNT.h>
#include <Methane/Data/Emitter.hpp>

#include <memory>

namespace Methane::Graphics
{

class DeviceBase;
struct CommandQueue;
class CommandQueueBase;

class ContextBase
    : public ObjectBase
    , public virtual Context
    , public IContextNT
    , public Data::Emitter<IContextCallback>
{
public:
    ContextBase(DeviceBase& device, Type type);

    // Context interface
    Type             GetType() const override { return m_type; }
    void             CompleteInitialization() override;
    void             WaitForGpu(WaitFor wait_for) override;
    void             Reset(Device& device) override;
    void             Reset() override;
    CommandQueue&    GetUploadCommandQueue() override;
    BlitCommandList& GetUploadCommandList() override;
    CommandListSet&  GetUploadCommandListSet() override;
    Device&          GetDevice() override;

    // ContextBase interface
    virtual void Initialize(DeviceBase& device, bool deferred_heap_allocation, bool is_callback_emitted = true);
    virtual void Release();

    // Object interface
    void SetName(const std::string& name) override;

    void RequireCompleteInitialization() noexcept       { m_is_complete_initialization_required = true; }
    ResourceManager&        GetResourceManager()        { return m_resource_manager; }
    const ResourceManager&  GetResourceManager() const  { return m_resource_manager; }
    CommandQueueBase&       GetUploadCommandQueueBase();
    DeviceBase&             GetDeviceBase();
    const DeviceBase&       GetDeviceBase() const;

protected:
    void SetDevice(DeviceBase& device);
    Fence& GetUploadFence() const noexcept;

    // ContextBase interface
    virtual bool UploadResources();
    virtual void OnGpuWaitStart(WaitFor) {}
    virtual void OnGpuWaitComplete(WaitFor wait_for);

private:
    const Type                m_type;
    Ptr<DeviceBase>           m_sp_device;
    ResourceManager::Settings m_resource_manager_init_settings{ true, {}, {} };
    ResourceManager           m_resource_manager;
    Ptr<CommandQueue>         m_sp_upload_cmd_queue;
    Ptr<BlitCommandList>      m_sp_upload_cmd_list;
    Ptr<CommandListSet>       m_sp_upload_cmd_lists;
    Ptr<Fence>                m_sp_upload_fence;
    bool                      m_is_complete_initialization_required = false;
};

} // namespace Methane::Graphics
