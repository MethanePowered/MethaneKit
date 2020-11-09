/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License"),
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

namespace tf
{
// TaskFlow Executor class forward declaration:
// #include <taskflow/core/executor.hpp>
class Executor;
}

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
    ContextBase(DeviceBase& device, tf::Executor& parallel_executor, Type type);

    // Context interface
    Type              GetType() const noexcept override                       { return m_type; }
    tf::Executor&     GetParallelExecutor() const noexcept override           { return m_parallel_executor; }
    Object::Registry& GetObjectsRegistry() noexcept override                     { return m_objects_cache; }
    void              RequestDeferredAction(DeferredAction action) const noexcept override;
    void              CompleteInitialization() override;
    bool              IsCompletingInitialization() const noexcept override    { return m_is_completing_initialization; }
    void              WaitForGpu(WaitFor wait_for) override;
    void              Reset(Device& device) override;
    void              Reset() override;
    CommandQueue&     GetUploadCommandQueue() override;
    BlitCommandList&  GetUploadCommandList() override;
    CommandListSet&   GetUploadCommandListSet() override;
    Device&           GetDevice() override;

    // ContextBase interface
    virtual void Initialize(DeviceBase& device, bool deferred_heap_allocation, bool is_callback_emitted = true);
    virtual void Release();

    // Object interface
    void SetName(const std::string& name) override;

    DeferredAction          GetRequestedAction() const noexcept  { return m_requested_action; }
    ResourceManager&        GetResourceManager() noexcept        { return m_resource_manager; }
    const ResourceManager&  GetResourceManager() const noexcept  { return m_resource_manager; }
    CommandQueueBase&       GetUploadCommandQueueBase();
    DeviceBase&             GetDeviceBase();
    const DeviceBase&       GetDeviceBase() const;

protected:
    void PerformRequestedAction();
    void SetDevice(DeviceBase& device);
    Fence& GetUploadFence() const;

    // ContextBase interface
    virtual bool UploadResources();
    virtual void OnGpuWaitStart(WaitFor);
    virtual void OnGpuWaitComplete(WaitFor wait_for);

private:
    const Type                m_type;
    Ptr<DeviceBase>           m_device_ptr;
    tf::Executor&             m_parallel_executor;
    ObjectBase::RegistryBase  m_objects_cache;
    ResourceManager::Settings m_resource_manager_init_settings{ true, {}, {} };
    ResourceManager           m_resource_manager;
    Ptr<CommandQueue>         m_upload_cmd_queue_ptr;
    Ptr<BlitCommandList>      m_upload_cmd_list_ptr;
    Ptr<CommandListSet>       m_upload_cmd_lists_ptr;
    Ptr<Fence>                m_upload_fence_ptr;
    mutable DeferredAction    m_requested_action = DeferredAction::None;
    mutable bool              m_is_completing_initialization = false;
};

} // namespace Methane::Graphics
