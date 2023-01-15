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

namespace Methane::Graphics::META_GFX_NAME
{
class TransferCommandList;
}

namespace Methane::Graphics::Rhi
{

class CommandQueue;
class CommandListDebugGroup;

class TransferCommandList // NOSONAR - constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
{
public:
    using Type        = CommandListType;
    using State       = CommandListState;
    using DebugGroup  = CommandListDebugGroup;
    using ICallback   = ICommandListCallback;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(TransferCommandList);
    META_PIMPL_METHODS_COMPARE_DECLARE(TransferCommandList);

    META_RHI_API explicit TransferCommandList(const Ptr<ITransferCommandList>& interface_ptr);
    META_RHI_API explicit TransferCommandList(ITransferCommandList& interface_ref);
    META_RHI_API explicit TransferCommandList(const CommandQueue& command_queue);

    META_RHI_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_RHI_API ITransferCommandList& GetInterface() const META_PIMPL_NOEXCEPT;
    META_RHI_API Ptr<ITransferCommandList> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_RHI_API bool SetName(std::string_view name) const;
    META_RHI_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_RHI_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_RHI_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // ICommandList interface methods
    META_RHI_API void  PushDebugGroup(const DebugGroup& debug_group) const;
    META_RHI_API void  PopDebugGroup() const;
    META_RHI_API void  Reset(const DebugGroup* debug_group_ptr = nullptr) const;
    META_RHI_API void  ResetOnce(const DebugGroup* debug_group_ptr = nullptr) const;
    META_RHI_API void  SetResourceBarriers(const IResourceBarriers& resource_barriers) const;
    META_RHI_API void  Commit() const;
    META_RHI_API void  WaitUntilCompleted(uint32_t timeout_ms = 0U) const;
    [[nodiscard]] META_RHI_API Data::TimeRange GetGpuTimeRange(bool in_cpu_nanoseconds) const;
    [[nodiscard]] META_RHI_API State GetState() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API CommandQueue GetCommandQueue() const;

    // Data::IEmitter<ICommandListCallback> interface methods
    META_RHI_API void Connect(Data::Receiver<ICommandListCallback>& receiver) const;
    META_RHI_API void Disconnect(Data::Receiver<ICommandListCallback>& receiver) const;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::TransferCommandList;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_RHI_PIMPL_INLINE

#include <Methane/Graphics/RHI/TransferCommandList.cpp>

#endif // META_RHI_PIMPL_INLINE
