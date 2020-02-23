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
{
public:
    ContextBase(DeviceBase& device, Type type);

    // Context interface
    Type             GetType() const override { return m_type; }
    void             CompleteInitialization() override;
    void             WaitForGpu(WaitFor wait_for) override;
    void             Reset(Device& device) override;
    void             Reset() override;
    void             AddCallback(Callback& callback) override;
    void             RemoveCallback(Callback& callback) override;
    CommandQueue&    GetUploadCommandQueue() override;
    BlitCommandList& GetUploadCommandList() override;
    Device&          GetDevice() override;

    // ContextBase interface
    virtual void Initialize(DeviceBase& device, bool deferred_heap_allocation);
    virtual void Release();

    // Object interface
    void SetName(const std::string& name) override;

    ResourceManager&        GetResourceManager()        { return m_resource_manager; }
    const ResourceManager&  GetResourceManager() const  { return m_resource_manager; }
    CommandQueueBase&       GetUploadCommandQueueBase();
    DeviceBase&             GetDeviceBase();
    const DeviceBase&       GetDeviceBase() const;

protected:
    void UploadResources();
    void SetDevice(DeviceBase& device);

    // ContextBase interface
    virtual void OnGpuWaitStart(WaitFor wait_for) {}
    virtual void OnGpuWaitComplete(WaitFor wait_for);

private:
    const Type                m_type;
    Ptr<DeviceBase>           m_sp_device;
    ResourceManager::Settings m_resource_manager_init_settings = { true };
    ResourceManager           m_resource_manager;
    Refs<Callback>            m_callbacks; // ORDER: Keep callbacks before resources for correct auto-delete
    Ptr<CommandQueue>         m_sp_upload_cmd_queue;
    Ptr<BlitCommandList>      m_sp_upload_cmd_list;
    UniquePtr<Fence>          m_sp_upload_fence;
};

} // namespace Methane::Graphics
