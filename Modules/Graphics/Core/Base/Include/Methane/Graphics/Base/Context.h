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

FILE: Methane/Graphics/Base/Context.h
Base implementation of the context interface.

******************************************************************************/

#pragma once

#include "Object.h"

#include <Methane/Graphics/IFence.h>
#include <Methane/Graphics/IContext.h>
#include <Methane/Graphics/ICommandKit.h>
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

struct ICommandQueue;
struct ICommandList;
struct ICommandListSet;
struct IDescriptorManager;

} // namespace Methane::Graphics

namespace Methane::Graphics::Base
{

class Device;
class CommandQueue;

class Context
    : public Object
    , public virtual IContext // NOSONAR
    , public Data::Emitter<IContextCallback>
{
public:
    Context(Device& device, UniquePtr<IDescriptorManager>&& descriptor_manager_ptr,
                tf::Executor& parallel_executor, Type type);
    ~Context() override;

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

    // Context interface
    virtual void Initialize(Device& device, bool is_callback_emitted = true);
    virtual void Release();

    // IObject interface
    bool SetName(const std::string& name) override;

    DeferredAction      GetRequestedAction() const noexcept { return m_requested_action; }
    Ptr<Device>         GetBaseDevicePtr() const noexcept   { return m_device_ptr; }
    Device&             GetBaseDevice();
    const Device&       GetBaseDevice() const;
    IDescriptorManager& GetDescriptorManager() const;

protected:
    void PerformRequestedAction();
    void SetDevice(Device& device);

    // Context interface
    virtual bool UploadResources();
    virtual void OnGpuWaitStart(WaitFor);
    virtual void OnGpuWaitComplete(WaitFor wait_for);

private:
    using CommandKitPtrByType = std::array<Ptr<ICommandKit>, magic_enum::enum_count<CommandListType>()>;
    using CommandKitByQueue   = std::map<ICommandQueue*, Ptr<ICommandKit>>;

    template<CommandListPurpose cmd_list_purpose>
    void ExecuteSyncCommandLists(const ICommandKit& upload_cmd_kit) const;

    const Type                   m_type;
    Ptr<Device>                   m_device_ptr;
    UniquePtr<IDescriptorManager> m_descriptor_manager_ptr;
    tf::Executor&                 m_parallel_executor;
    ObjectRegistry                m_objects_cache;
    mutable CommandKitPtrByType   m_default_command_kit_ptrs;
    mutable CommandKitByQueue     m_default_command_kit_ptr_by_queue;
    mutable DeferredAction        m_requested_action = DeferredAction::None;
    mutable bool                  m_is_completing_initialization = false;
};

} // namespace Methane::Graphics::Base
