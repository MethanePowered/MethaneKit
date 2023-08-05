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

FILE: Methane/Graphics/Base/ParallelRenderCommandList.cpp
Base implementation of the parallel render command list interface.

******************************************************************************/

#include <Methane/Graphics/Base/ParallelRenderCommandList.h>
#include <Methane/Graphics/Base/RenderCommandList.h>
#include <Methane/Graphics/Base/RenderPass.h>
#include <Methane/Graphics/Base/RenderState.h>
#include <Methane/Graphics/Base/Buffer.h>
#include <Methane/Graphics/Base/Program.h>
#include <Methane/Graphics/Base/CommandQueue.h>
#include <Methane/Graphics/Base/Context.h>

#include <Methane/Graphics/RHI/ICommandListDebugGroup.h>

#include <Methane/Instrumentation.h>

#include <taskflow/algorithm/for_each.hpp>
#include <fmt/format.h>

#include <string_view>

namespace Methane::Graphics::Base
{

ParallelRenderCommandList::ParallelRenderCommandList(CommandQueue& command_queue, RenderPass& render_pass)
    : CommandList(command_queue, Type::ParallelRender)
    , m_render_pass_ptr(render_pass.GetPtr<RenderPass>())
{ }

void ParallelRenderCommandList::SetValidationEnabled(bool is_validation_enabled)
{
    META_FUNCTION_TASK();
    m_is_validation_enabled = is_validation_enabled;
    for(const Ptr<RenderCommandList>& render_command_list_ptr : m_parallel_command_lists)
    {
        META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
        render_command_list_ptr->SetValidationEnabled(m_is_validation_enabled);
    }
}

void ParallelRenderCommandList::Reset(IDebugGroup* debug_group_ptr)
{
    META_FUNCTION_TASK();
    ResetImpl(debug_group_ptr, [this, debug_group_ptr](const Data::Index command_list_index)
    {
        META_FUNCTION_TASK();
        const Ptr<RenderCommandList>& render_command_list_ptr = m_parallel_command_lists[command_list_index];
        META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
        render_command_list_ptr->Reset(debug_group_ptr ? debug_group_ptr->GetSubGroup(command_list_index) : nullptr);
    });
}

void ParallelRenderCommandList::ResetWithState(Rhi::IRenderState& render_state, IDebugGroup* debug_group_ptr)
{
    META_FUNCTION_TASK();
    ResetImpl(debug_group_ptr, [this, &render_state, debug_group_ptr](Data::Index command_list_index)
    {
        META_FUNCTION_TASK();
        const Ptr<RenderCommandList>& render_command_list_ptr = m_parallel_command_lists[command_list_index];
        META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
        render_command_list_ptr->ResetWithState(render_state, debug_group_ptr ? debug_group_ptr->GetSubGroup(command_list_index) : nullptr);
    });
}

template<typename ResetCommandListFn>
void ParallelRenderCommandList::ResetImpl(IDebugGroup* debug_group_ptr, const ResetCommandListFn& reset_command_list_fn) // NOSONAR - function can not be const
{
    CommandList::Reset();

    // Create per-thread debug sub-group:
    if (debug_group_ptr && !debug_group_ptr->HasSubGroups())
    {
        for(Data::Index render_command_list_index = 0; render_command_list_index < m_parallel_command_lists.size(); ++render_command_list_index)
        {
            debug_group_ptr->AddSubGroup(render_command_list_index, GetThreadCommandListName(debug_group_ptr->GetName(), render_command_list_index));
        }
    }

    // Per-thread render command lists can be reset in parallel only with DirectX 12 on Windows
#ifdef _WIN32
    tf::Taskflow reset_task_flow;
    reset_task_flow.for_each_index(0U, static_cast<uint32_t>(m_parallel_command_lists.size()), 1U, reset_command_list_fn);
    GetCommandQueue().GetContext().GetParallelExecutor().run(reset_task_flow).get();
#else
    for(Data::Index command_list_index = 0U; command_list_index < static_cast<Data::Index>(m_parallel_command_lists.size()); ++command_list_index)
        reset_command_list_fn(command_list_index);
#endif
}

void ParallelRenderCommandList::Commit()
{
    META_FUNCTION_TASK();
    tf::Taskflow commit_task_flow;
    commit_task_flow.for_each(m_parallel_command_lists.begin(), m_parallel_command_lists.end(),
        [](const Ptr<RenderCommandList>& render_command_list_ptr)
        {
            META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
            render_command_list_ptr->Commit();
        }
    );
    GetCommandQueue().GetContext().GetParallelExecutor().run(commit_task_flow).get();
    CommandList::Commit();
}

void ParallelRenderCommandList::SetViewState(Rhi::IViewState& view_state)
{
    META_FUNCTION_TASK();
    for(const Ptr<RenderCommandList>& render_command_list_ptr : m_parallel_command_lists)
    {
        META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
        render_command_list_ptr->SetViewState(view_state);
    }
}

void ParallelRenderCommandList::SetParallelCommandListsCount(uint32_t count)
{
    META_FUNCTION_TASK();
    const auto initial_count = static_cast<uint32_t>(m_parallel_command_lists.size());
    if (count < initial_count)
    {
        m_parallel_command_lists.erase(m_parallel_command_lists.begin() + count);
        m_parallel_command_lists_refs.erase(m_parallel_command_lists_refs.begin() + count);
        return;
    }

    const std::string_view name = GetName();
    m_parallel_command_lists.reserve(count);
    m_parallel_command_lists_refs.reserve(count);

    for(uint32_t cmd_list_index = initial_count; cmd_list_index < count; ++cmd_list_index)
    {
        m_parallel_command_lists.emplace_back(std::static_pointer_cast<RenderCommandList>(CreateCommandList(false)));
        RenderCommandList& render_command_list = *m_parallel_command_lists.back();
        render_command_list.SetValidationEnabled(m_is_validation_enabled);
        m_parallel_command_lists_refs.emplace_back(render_command_list);
        if (!name.empty())
        {
            render_command_list.SetName(GetThreadCommandListName(name, cmd_list_index));
        }
    }
}

void ParallelRenderCommandList::Execute(const Rhi::ICommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    for(const Ptr<RenderCommandList>& render_command_list_ptr : m_parallel_command_lists)
    {
        META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
        render_command_list_ptr->Execute();
    }

    CommandList::Execute(completed_callback);
}

void ParallelRenderCommandList::Complete()
{
    META_FUNCTION_TASK();
    for(const Ptr<RenderCommandList>& render_command_list_ptr : m_parallel_command_lists)
    {
        META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
        render_command_list_ptr->Complete();
    }

    CommandList::Complete();
}

bool ParallelRenderCommandList::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!CommandList::SetName(name) || name.empty())
        return false;

    uint32_t render_cmd_list_index = 0;
    for(const Ptr<RenderCommandList>& render_cmd_list_ptr : m_parallel_command_lists)
    {
        META_CHECK_ARG_NOT_NULL(render_cmd_list_ptr);
        render_cmd_list_ptr->SetName(GetThreadCommandListName(name, render_cmd_list_index));
        render_cmd_list_index++;
    }
    return true;
}

RenderPass& ParallelRenderCommandList::GetRenderPass() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_render_pass_ptr);
    return *m_render_pass_ptr;
}

std::string ParallelRenderCommandList::GetParallelCommandListDebugName(std::string_view base_name, std::string_view suffix)
{
    return base_name.empty() ? std::string() : fmt::format("{} {}", base_name, suffix);
}

std::string ParallelRenderCommandList::GetTrailingCommandListDebugName(std::string_view base_name, bool is_beginning)
{
    return GetParallelCommandListDebugName(base_name, is_beginning ? "[Beginning]" : "[Ending]");
}

std::string ParallelRenderCommandList::GetThreadCommandListName(std::string_view base_name, Data::Index index)
{
    return GetParallelCommandListDebugName(base_name, fmt::format("- Thread {}", index));
}

} // namespace Methane::Graphics::Base
