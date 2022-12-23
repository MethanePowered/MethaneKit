/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/TransferCommandList.h
Methane TransferCommandList PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include "Pimpl.h"

#include <Methane/Graphics/RHI/ITransferCommandList.h>
#include <Methane/Data/Transmitter.hpp>

namespace Methane::Graphics::Rhi
{

class CommandQueue;
class CommandListDebugGroup;

class TransferCommandList
    : public Data::Transmitter<Rhi::ICommandListCallback>
    , public Data::Transmitter<Rhi::IObjectCallback>
{
public:
    using Type        = CommandListType;
    using State       = CommandListState;
    using DebugGroup  = CommandListDebugGroup;
    using ICallback   = ICommandListCallback;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(TransferCommandList);

    TransferCommandList(const Ptr<ITransferCommandList>& interface_ptr);
    TransferCommandList(ITransferCommandList& interface_ref);
    TransferCommandList(const CommandQueue& command_queue);

    void Init(const CommandQueue& command_queue);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    ITransferCommandList& GetInterface() const META_PIMPL_NOEXCEPT;
    Ptr<ITransferCommandList> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    bool SetName(std::string_view name) const;
    std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // ICommandList interface methods
    void  PushDebugGroup(DebugGroup& debug_group);
    void  PopDebugGroup();
    void  Reset(DebugGroup* debug_group_ptr = nullptr);
    void  ResetOnce(DebugGroup* debug_group_ptr = nullptr);
    void  SetProgramBindings(IProgramBindings& program_bindings,
                             ProgramBindingsApplyBehaviorMask apply_behavior = ProgramBindingsApplyBehaviorMask(~0U));
    void  SetResourceBarriers(const IResourceBarriers& resource_barriers);
    void  Commit();
    void  WaitUntilCompleted(uint32_t timeout_ms = 0U);
    [[nodiscard]] Data::TimeRange GetGpuTimeRange(bool in_cpu_nanoseconds) const;
    [[nodiscard]] State GetState() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] CommandQueue GetCommandQueue();

private:
    class Impl;

    TransferCommandList(UniquePtr<Impl>&& impl_ptr);

    UniquePtr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi
