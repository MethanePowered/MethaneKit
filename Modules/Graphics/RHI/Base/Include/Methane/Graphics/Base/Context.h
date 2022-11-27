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

#include <Methane/Graphics/RHI/IFence.h>
#include <Methane/Graphics/RHI/IContext.h>
#include <Methane/Graphics/RHI/ICommandKit.h>
#include <Methane/Data/Emitter.hpp>

#include <array>
#include <string>

namespace tf
{
// TaskFlow Executor class forward declaration:
// #include <taskflow/core/executor.hpp>
class Executor;
}

namespace Methane::Graphics::Rhi
{

struct ICommandQueue;
struct ICommandList;
struct ICommandListSet;
struct IDescriptorManager;

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Base
{

class Device;
class CommandQueue;

class Context
    : public Object
    , public virtual Rhi::IContext // NOSONAR
    , public Data::Emitter<Rhi::IContextCallback>
{
public:
    Context(Device& device,
            UniquePtr<Rhi::IDescriptorManager>&& descriptor_manager_ptr,
            tf::Executor& parallel_executor, Type type);
    ~Context() override;

    // IContext interface
    Type                        GetType() const noexcept override                       { return m_type; }
    tf::Executor&               GetParallelExecutor() const noexcept override           { return m_parallel_executor; }
    Rhi::IObjectRegistry&       GetObjectRegistry() noexcept override                   { return m_objects_cache; }
    const Rhi::IObjectRegistry& GetObjectRegistry() const noexcept override             { return m_objects_cache; }
    void                        RequestDeferredAction(DeferredAction action) const noexcept override;
    void                        CompleteInitialization() override;
    bool                        IsCompletingInitialization() const noexcept override    { return m_is_completing_initialization; }
    void                        WaitForGpu(WaitFor wait_for) override;
    void                        Reset(Rhi::IDevice& device) override;
    void                        Reset() override;
    Rhi::ICommandKit&           GetDefaultCommandKit(Rhi::CommandListType type) const final;
    Rhi::ICommandKit&           GetDefaultCommandKit(Rhi::ICommandQueue& cmd_queue) const final;
    const Rhi::IDevice&         GetDevice() const final;

    // Context interface
    virtual void Initialize(Device& device, bool is_callback_emitted = true);
    virtual void Release();

    // IObject interface
    bool SetName(const std::string& name) override;

    DeferredAction      GetRequestedAction() const noexcept { return m_requested_action; }
    Ptr<Device>         GetBaseDevicePtr() const noexcept   { return m_device_ptr; }
    Device&             GetBaseDevice();
    const Device&       GetBaseDevice() const;
    Rhi::IDescriptorManager& GetDescriptorManager() const;

protected:
    void PerformRequestedAction();
    void SetDevice(Device& device);

    // Context interface
    virtual bool UploadResources();
    virtual void OnGpuWaitStart(WaitFor);
    virtual void OnGpuWaitComplete(WaitFor wait_for);

private:
    using CommandKitPtrByType = std::array<Ptr<Rhi::ICommandKit>, static_cast<size_t>(Rhi::CommandListType::Count)>;
    using CommandKitByQueue   = std::map<Rhi::ICommandQueue*, Ptr<Rhi::ICommandKit>>;

    template<Rhi::CommandListPurpose cmd_list_purpose>
    void ExecuteSyncCommandLists(const Rhi::ICommandKit& upload_cmd_kit) const;

    const Type                         m_type;
    Ptr<Device>                        m_device_ptr;
    UniquePtr<Rhi::IDescriptorManager> m_descriptor_manager_ptr;
    tf::Executor&                      m_parallel_executor;
    ObjectRegistry                     m_objects_cache;
    mutable CommandKitPtrByType        m_default_command_kit_ptrs;
    mutable CommandKitByQueue          m_default_command_kit_ptr_by_queue;
    mutable DeferredAction             m_requested_action = DeferredAction::None;
    mutable bool                       m_is_completing_initialization = false;
};

} // namespace Methane::Graphics::Base
