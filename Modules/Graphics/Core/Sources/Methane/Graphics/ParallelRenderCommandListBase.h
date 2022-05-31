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

FILE: Methane/Graphics/ParallelRenderCommandListBase.h
Base implementation of the parallel render command list interface.

******************************************************************************/

#pragma once

#include "CommandListBase.h"
#include "RenderPassBase.h"

#include <Methane/Graphics/ParallelRenderCommandList.h>

#include <optional>

namespace Methane::Graphics
{

struct RenderState;

class ParallelRenderCommandListBase
    : public ParallelRenderCommandList
    , public CommandListBase
{
public:
    ParallelRenderCommandListBase(CommandQueueBase& command_queue, RenderPassBase& render_pass);
    
    using CommandListBase::Reset;

    // ParallelRenderCommandList interface
    bool IsValidationEnabled() const noexcept override { return m_is_validation_enabled; }
    void SetValidationEnabled(bool is_validation_enabled) override;
    void Reset(DebugGroup* p_debug_group = nullptr) override;
    void ResetWithState(RenderState& render_state, DebugGroup* p_debug_group = nullptr) override;
    void SetViewState(ViewState& view_state) override;
    void SetParallelCommandListsCount(uint32_t count) override;
    const Refs<RenderCommandList>& GetParallelCommandLists() const override { return m_parallel_command_lists_refs; }

    // CommandListBase interface
    void SetResourceBarriers(const Resource::Barriers&) override { META_FUNCTION_NOT_IMPLEMENTED_DESCR("Can not set resource barriers on parallel render command list."); }
    void Execute(const CommandList::CompletedCallback& completed_callback) override;
    void Complete() override;

    // CommandList interface
    void PushDebugGroup(DebugGroup&) override   { META_FUNCTION_NOT_IMPLEMENTED_DESCR("Can not use debug groups on parallel render command list."); }
    void PopDebugGroup() override               { META_FUNCTION_NOT_IMPLEMENTED_DESCR("Can not use debug groups on parallel render command list."); }
    void Commit() override;

    // Object interface
    bool SetName(const std::string& name) override;

    RenderPassBase& GetPass();

private:
    template<typename ResetCommandListFn>
    void ResetImpl(DebugGroup* p_debug_group, const ResetCommandListFn& reset_command_list_fn);

    const Ptr<RenderPassBase>   m_render_pass_ptr;
    Ptrs<RenderCommandListBase> m_parallel_command_lists;
    Refs<RenderCommandList>     m_parallel_command_lists_refs;
    bool                        m_is_validation_enabled = true;
};

} // namespace Methane::Graphics
