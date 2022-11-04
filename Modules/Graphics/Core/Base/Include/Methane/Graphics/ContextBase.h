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

#include <Methane/Graphics/ObjectBase.h>

#include <Methane/Graphics/IFence.h>
#include <Methane/Graphics/IContext.h>
#include <Methane/Graphics/ICommandKit.h>
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
struct ICommandQueue;
struct ICommandList;
struct ICommandListSet;
struct DescriptorManager;

class ContextBase
    : public ObjectBase
    , public virtual IContext // NOSONAR
    , public IContextNT
    , public Data::Emitter<IContextCallback>
{
public:
    ContextBase(DeviceBase& device, UniquePtr<DescriptorManager>&& descriptor_manager_ptr,
                tf::Executor& parallel_executor, Type type);
    ~ContextBase() override;

    // IContext interface
    Type              GetType() const noexcept override                       { return m_type; }
    tf::Executor&     GetParallelExecutor() const noexcept override           { return m_parallel_executor; }
    IObjectRegistry&  GetObjectRegistry() noexcept override                   { return m_objects_cache; }
    const IObjectRegistry& GetObjectRegistry() const noexcept override        { return m_objects_cache; }
    void              RequestDeferredAction(DeferredAction action) const noexcept override;
    void              CompleteInitialization() override;
    bool              IsCompletingInitialization() const noexcept override    { return m_is_completing_initialization; }
    void              WaitForGpu(WaitFor wait_for) override;
    void              Reset(IDevice& device) override;
    void              Reset() override;
    ICommandKit&      GetDefaultCommandKit(CommandListType type) const final;
    ICommandKit&      GetDefaultCommandKit(ICommandQueue& cmd_queue) const final;
    const IDevice&    GetDevice() const final;

    // ContextBase interface
    virtual void Initialize(DeviceBase& device, bool is_callback_emitted = true);
    virtual void Release();

    // IObject interface
    bool SetName(const std::string& name) override;

    DeferredAction     GetRequestedAction() const noexcept { return m_requested_action; }
    Ptr<DeviceBase>    GetDeviceBasePtr() const noexcept   { return m_device_ptr; }
    DeviceBase&        GetDeviceBase();
    const DeviceBase&  GetDeviceBase() const;
    DescriptorManager& GetDescriptorManager() const;

protected:
    void PerformRequestedAction();
    void SetDevice(DeviceBase& device);

    // ContextBase interface
    virtual bool UploadResources();
    virtual void OnGpuWaitStart(WaitFor);
    virtual void OnGpuWaitComplete(WaitFor wait_for);

private:
    using CommandKitPtrByType = std::array<Ptr<ICommandKit>, magic_enum::enum_count<CommandListType>()>;
    using CommandKitByQueue   = std::map<ICommandQueue*, Ptr<ICommandKit>>;

    template<CommandListPurpose cmd_list_purpose>
    void ExecuteSyncCommandLists(const ICommandKit& upload_cmd_kit) const;

    const Type                   m_type;
    Ptr<DeviceBase>              m_device_ptr;
    UniquePtr<DescriptorManager> m_descriptor_manager_ptr;
    tf::Executor&                m_parallel_executor;
    ObjectRegistryBase           m_objects_cache;
    mutable CommandKitPtrByType  m_default_command_kit_ptrs;
    mutable CommandKitByQueue    m_default_command_kit_ptr_by_queue;
    mutable DeferredAction       m_requested_action = DeferredAction::None;
    mutable bool                 m_is_completing_initialization = false;
};

} // namespace Methane::Graphics
