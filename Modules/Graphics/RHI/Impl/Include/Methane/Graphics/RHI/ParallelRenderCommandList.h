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

FILE: Methane/Graphics/RHI/ParallelRenderCommandList.h
Methane ParallelRenderCommandList PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include <Methane/Pimpl.h>
#include "RenderCommandList.h"

#include <Methane/Graphics/RHI/IParallelRenderCommandList.h>

namespace Methane::Graphics::META_GFX_NAME
{
class ParallelRenderCommandList;
}

namespace Methane::Graphics::Rhi
{

class CommandQueue;
class CommandListDebugGroup;
class ResourceBarriers;
class RenderPass;
class RenderState;
class ViewState;
class RenderCommandList;

class ParallelRenderCommandList // NOSONAR - class has more than 35 methods, constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
{
public:
    using Type        = CommandListType;
    using State       = CommandListState;
    using DebugGroup  = CommandListDebugGroup;
    using ICallback   = ICommandListCallback;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(ParallelRenderCommandList);
    META_PIMPL_METHODS_COMPARE_DECLARE(ParallelRenderCommandList);

    META_PIMPL_API explicit ParallelRenderCommandList(const Ptr<IParallelRenderCommandList>& interface_ptr);
    META_PIMPL_API explicit ParallelRenderCommandList(IParallelRenderCommandList& interface_ref);
    META_PIMPL_API ParallelRenderCommandList(const CommandQueue& command_queue, const RenderPass& render_pass);

    META_PIMPL_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API IParallelRenderCommandList& GetInterface() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API Ptr<IParallelRenderCommandList> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_PIMPL_API bool SetName(std::string_view name) const;
    META_PIMPL_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_PIMPL_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_PIMPL_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // ICommandList interface methods
    META_PIMPL_API void  PushDebugGroup(const DebugGroup& debug_group) const;
    META_PIMPL_API void  PopDebugGroup() const;
    META_PIMPL_API void  Reset(const DebugGroup* debug_group_ptr = nullptr) const;
    META_PIMPL_API void  ResetOnce(const DebugGroup* debug_group_ptr = nullptr) const;
    META_PIMPL_API void  SetProgramBindings(IProgramBindings& program_bindings,
                                            ProgramBindingsApplyBehaviorMask apply_behavior = ProgramBindingsApplyBehaviorMask(~0U)) const;
    META_PIMPL_API void  SetResourceBarriers(const ResourceBarriers& resource_barriers) const;
    META_PIMPL_API void  Commit() const;
    META_PIMPL_API void  WaitUntilCompleted(uint32_t timeout_ms = 0U) const;
    [[nodiscard]] META_PIMPL_API Data::TimeRange GetGpuTimeRange(bool in_cpu_nanoseconds) const;
    [[nodiscard]] META_PIMPL_API State GetState() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API CommandQueue GetCommandQueue() const;

    // Data::IEmitter<ICommandListCallback> interface methods
    META_PIMPL_API void Connect(Data::Receiver<ICommandListCallback>& receiver) const;
    META_PIMPL_API void Disconnect(Data::Receiver<ICommandListCallback>& receiver) const;

    // IParallelRenderCommandList interface methods
    [[nodiscard]] META_PIMPL_API bool IsValidationEnabled() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API void SetValidationEnabled(bool is_validation_enabled) const;
    META_PIMPL_API void ResetWithState(const RenderState& render_state, const DebugGroup* debug_group_ptr = nullptr) const;
    META_PIMPL_API void SetViewState(const ViewState& view_state) const;
    META_PIMPL_API void SetBeginningResourceBarriers(const ResourceBarriers& resource_barriers) const;
    META_PIMPL_API void SetEndingResourceBarriers(const ResourceBarriers& resource_barriers) const;
    META_PIMPL_API void SetParallelCommandListsCount(uint32_t count) const;
    [[nodiscard]] META_PIMPL_API const std::vector<RenderCommandList>& GetParallelCommandLists() const;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::ParallelRenderCommandList;

    Ptr<Impl> m_impl_ptr;
    mutable std::vector<RenderCommandList> m_parallel_command_lists;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_PIMPL_INLINE

#include <Methane/Graphics/RHI/ParallelRenderCommandList.cpp>

#endif // META_PIMPL_INLINE
