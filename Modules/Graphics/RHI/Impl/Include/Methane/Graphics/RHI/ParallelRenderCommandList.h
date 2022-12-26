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

#include "Pimpl.h"
#include "RenderCommandList.h"

#include <Methane/Graphics/RHI/IParallelRenderCommandList.h>
#include <Methane/Data/Transmitter.hpp>

namespace Methane::Graphics::METHANE_GFX_API
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

class ParallelRenderCommandList
    : public Data::Transmitter<Rhi::ICommandListCallback>
    , public Data::Transmitter<Rhi::IObjectCallback>
{
public:
    using Type        = CommandListType;
    using State       = CommandListState;
    using DebugGroup  = CommandListDebugGroup;
    using ICallback   = ICommandListCallback;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(ParallelRenderCommandList);
    META_PIMPL_METHODS_COMPARE_DECLARE(ParallelRenderCommandList);

    explicit ParallelRenderCommandList(const Ptr<IParallelRenderCommandList>& interface_ptr);
    explicit ParallelRenderCommandList(IParallelRenderCommandList& interface_ref);
    ParallelRenderCommandList(const CommandQueue& command_queue, const RenderPass& render_pass);

    void Init(const CommandQueue& command_queue, const RenderPass& render_pass);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    IParallelRenderCommandList& GetInterface() const META_PIMPL_NOEXCEPT;
    Ptr<IParallelRenderCommandList> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

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
    void  SetResourceBarriers(const ResourceBarriers& resource_barriers) const;
    void  Commit() const;
    void  WaitUntilCompleted(uint32_t timeout_ms = 0U) const;
    [[nodiscard]] Data::TimeRange GetGpuTimeRange(bool in_cpu_nanoseconds) const;
    [[nodiscard]] State GetState() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] CommandQueue GetCommandQueue() const;

    // IParallelRenderCommandList interface methods
    [[nodiscard]] bool IsValidationEnabled() const META_PIMPL_NOEXCEPT;
    void SetValidationEnabled(bool is_validation_enabled) const;
    void ResetWithState(const RenderState& render_state, const DebugGroup* debug_group_ptr = nullptr) const;
    void SetViewState(const ViewState& view_state) const;
    void SetBeginningResourceBarriers(const ResourceBarriers& resource_barriers) const;
    void SetEndingResourceBarriers(const ResourceBarriers& resource_barriers) const;
    void SetParallelCommandListsCount(uint32_t count) const;
    [[nodiscard]] const std::vector<RenderCommandList>& GetParallelCommandLists() const;

private:
    using Impl = Methane::Graphics::METHANE_GFX_API::ParallelRenderCommandList;

    ParallelRenderCommandList(Ptr<Impl>&& impl_ptr);

    Ptr<Impl> m_impl_ptr;
    mutable std::vector<RenderCommandList> m_parallel_command_lists;
};

} // namespace Methane::Graphics::Rhi
