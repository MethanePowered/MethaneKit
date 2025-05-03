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

FILE: Methane/Graphics/Base/ParallelRenderCommandList.h
Base implementation of the parallel render command list interface.

******************************************************************************/

#pragma once

#include "CommandList.h"
#include "RenderPass.h"

#include <Methane/Graphics/RHI/IParallelRenderCommandList.h>

#include <string>
#include <string_view>

namespace Methane::Graphics::Rhi
{

struct IRenderState;

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Base
{

class ParallelRenderCommandList
    : public Rhi::IParallelRenderCommandList
    , public CommandList
{
public:
    ParallelRenderCommandList(CommandQueue& command_queue, RenderPass& render_pass);
    
    using CommandList::Reset;

    // IParallelRenderCommandList interface
    [[nodiscard]] bool IsValidationEnabled() const noexcept final { return m_is_validation_enabled; }
    void SetValidationEnabled(bool is_validation_enabled) override;
    [[nodiscard]] Rhi::IRenderPass& GetRenderPass() const final;
    void Reset(IDebugGroup* debug_group_ptr = nullptr) override;
    void ResetWithState(Rhi::IRenderState& render_state, IDebugGroup* debug_group_ptr = nullptr) override;
    void SetViewState(Rhi::IViewState& view_state) override;
    void SetParallelCommandListsCount(uint32_t count) override;
    [[nodiscard]] const Refs<Rhi::IRenderCommandList>& GetParallelCommandLists() const override { return m_parallel_command_lists_refs; }

    // CommandList interface, which throw NotImplementedException (i.e. can not be used)
    void SetProgramBindings(Rhi::IProgramBindings&, Rhi::ProgramBindingsApplyBehaviorMask) override;
    void SetResourceBarriers(const Rhi::IResourceBarriers&) override;
    void PushDebugGroup(IDebugGroup&) override;
    void PopDebugGroup() override;

    // CommandList interface
    void Execute(const ICommandList::CompletedCallback& completed_callback = {}) override;
    void Complete() override;
    void Commit() override;

    // IObject interface
    bool SetName(std::string_view name) override;

    [[nodiscard]] RenderPass& GetBaseRenderPass() const;
    [[nodiscard]] const Ptr<RenderPass>& GetBaseRenderPassPtr() const noexcept { return m_render_pass_ptr;}

protected:
    static std::string GetParallelCommandListDebugName(std::string_view base_name, std::string_view suffix);
    static std::string GetTrailingCommandListDebugName(std::string_view base_name, bool is_beginning);
    static std::string GetThreadCommandListName(std::string_view base_name, Data::Index index);

    // ParallelRenderCommandListBase interface
    [[nodiscard]] virtual Ptr<Rhi::IRenderCommandList> CreateCommandList(bool is_beginning_list) = 0;

private:
    template<typename ResetCommandListFn>
    void ResetImpl(IDebugGroup* debug_group_ptr, const ResetCommandListFn& reset_command_list_fn);

    const Ptr<RenderPass>         m_render_pass_ptr;
    Ptrs<RenderCommandList>       m_parallel_command_lists;
    Refs<Rhi::IRenderCommandList> m_parallel_command_lists_refs;
    bool                          m_is_validation_enabled = true;
};

} // namespace Methane::Graphics::Base
