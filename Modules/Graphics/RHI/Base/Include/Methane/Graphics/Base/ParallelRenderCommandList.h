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

#include <optional>
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
    bool IsValidationEnabled() const noexcept override { return m_is_validation_enabled; }
    void SetValidationEnabled(bool is_validation_enabled) override;
    void Reset(IDebugGroup* debug_group_ptr = nullptr) override;
    void ResetWithState(Rhi::IRenderState& render_state, IDebugGroup* debug_group_ptr = nullptr) override;
    void SetViewState(Rhi::IViewState& view_state) override;
    void SetParallelCommandListsCount(uint32_t count) override;
    const Refs<Rhi::IRenderCommandList>& GetParallelCommandLists() const override { return m_parallel_command_lists_refs; }

    // CommandList interface
    void SetResourceBarriers(const Rhi::IResourceBarriers&) override { META_FUNCTION_NOT_IMPLEMENTED_DESCR("Can not set resource barriers on parallel render command list."); }
    void Execute(const ICommandList::CompletedCallback& completed_callback) override;
    void Complete() override;

    // ICommandList interface
    void PushDebugGroup(IDebugGroup&) override  { META_FUNCTION_NOT_IMPLEMENTED_DESCR("Can not use debug groups on parallel render command list."); }
    void PopDebugGroup() override               { META_FUNCTION_NOT_IMPLEMENTED_DESCR("Can not use debug groups on parallel render command list."); }
    void Commit() override;

    // IObject interface
    bool SetName(std::string_view name) override;

    RenderPass& GetRenderPass() const;

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
