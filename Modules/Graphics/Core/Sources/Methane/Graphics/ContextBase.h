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

#include <array>
#include <string>
#include <magic_enum.hpp>

namespace tf
{
// TaskFlow Executor class forward declaration:
// #include <taskflow/core/executor.hpp>
class Executor;
}

namespace Methane::Graphics
{

class DeviceBase;
class CommandQueueBase;
struct CommandQueue;
struct CommandList;
struct CommandListSet;

class ContextBase
    : public ObjectBase
    , public virtual Context // NOSONAR
    , public IContextNT
    , public Data::Emitter<IContextCallback>
{
public:
    ContextBase(DeviceBase& device, tf::Executor& parallel_executor, Type type);

    // Context interface
    Type              GetType() const noexcept override                       { return m_type; }
    tf::Executor&     GetParallelExecutor() const noexcept override           { return m_parallel_executor; }
    Object::Registry& GetObjectsRegistry() noexcept override                  { return m_objects_cache; }
    const Object::Registry& GetObjectsRegistry() const noexcept override      { return m_objects_cache; }
    void              RequestDeferredAction(DeferredAction action) const noexcept override;
    void              CompleteInitialization() override;
    bool              IsCompletingInitialization() const noexcept override    { return m_is_completing_initialization; }
    void              WaitForGpu(WaitFor wait_for) override;
    void              Reset(Device& device) override;
    void              Reset() override;
    CommandKit&       GetDefaultCommandKit(CommandList::Type type) const final;
    CommandKit&       GetDefaultCommandKit(CommandQueue& cmd_queue) const final;
    const Device&     GetDevice() const final;

    // ContextBase interface
    virtual void Initialize(DeviceBase& device, bool deferred_heap_allocation, bool is_callback_emitted = true);
    virtual void Release();

    // Object interface
    void SetName(const std::string& name) override;

    DeferredAction    GetRequestedAction() const noexcept  { return m_requested_action; }
    ResourceManager&  GetResourceManager() const noexcept  { return m_resource_manager; }
    DeviceBase&       GetDeviceBase();
    const DeviceBase& GetDeviceBase() const;

protected:
    void PerformRequestedAction();
    void SetDevice(DeviceBase& device);

    // ContextBase interface
    virtual bool UploadResources();
    virtual void OnGpuWaitStart(WaitFor);
    virtual void OnGpuWaitComplete(WaitFor wait_for);

private:
    using CommandKitPtrByType = std::array<Ptr<CommandKit>, magic_enum::enum_count<CommandList::Type>()>;
    using CommandKitByQueue   = std::map<CommandQueue*, Ptr<CommandKit>>;

    const Type                  m_type;
    Ptr<DeviceBase>             m_device_ptr;
    tf::Executor&               m_parallel_executor;
    ObjectBase::RegistryBase    m_objects_cache;
    ResourceManager::Settings   m_resource_manager_init_settings{ true, {}, {} };
    mutable ResourceManager     m_resource_manager;
    mutable CommandKitPtrByType m_default_command_kit_ptrs;
    mutable CommandKitByQueue   m_default_command_kit_ptr_by_queue;
    mutable DeferredAction      m_requested_action = DeferredAction::None;
    mutable bool                m_is_completing_initialization = false;
};

} // namespace Methane::Graphics
