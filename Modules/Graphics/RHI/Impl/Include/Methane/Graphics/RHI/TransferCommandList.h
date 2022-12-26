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

namespace Methane::Graphics::METHANE_GFX_API
{
class TransferCommandList;
}

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
    META_PIMPL_METHODS_COMPARE_DECLARE(TransferCommandList);

    explicit TransferCommandList(const Ptr<ITransferCommandList>& interface_ptr);
    explicit TransferCommandList(ITransferCommandList& interface_ref);
    explicit TransferCommandList(const CommandQueue& command_queue);

    void Init(const CommandQueue& command_queue);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    ITransferCommandList& GetInterface() const META_PIMPL_NOEXCEPT;
    Ptr<ITransferCommandList> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    bool SetName(std::string_view name) const;
    std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // ICommandList interface methods
    void  PushDebugGroup(DebugGroup& debug_group) const;
    void  PopDebugGroup() const;
    void  Reset(DebugGroup* debug_group_ptr = nullptr) const;
    void  ResetOnce(DebugGroup* debug_group_ptr = nullptr) const;
    void  SetProgramBindings(IProgramBindings& program_bindings,
                             ProgramBindingsApplyBehaviorMask apply_behavior = ProgramBindingsApplyBehaviorMask(~0U)) const;
    void  SetResourceBarriers(const IResourceBarriers& resource_barriers) const;
    void  Commit() const;
    void  WaitUntilCompleted(uint32_t timeout_ms = 0U) const;
    [[nodiscard]] Data::TimeRange GetGpuTimeRange(bool in_cpu_nanoseconds) const;
    [[nodiscard]] State GetState() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] CommandQueue GetCommandQueue() const;

private:
    using Impl = Methane::Graphics::METHANE_GFX_API::TransferCommandList;

    TransferCommandList(Ptr<Impl>&& impl_ptr);

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi
