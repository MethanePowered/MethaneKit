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

FILE: Methane/Graphics/ParallelRenderCommandListBase.cpp
Base implementation of the parallel render command list interface.

******************************************************************************/

#include "ParallelRenderCommandListBase.h"
#include "RenderCommandListBase.h"
#include "RenderPassBase.h"
#include "RenderStateBase.h"
#include "BufferBase.h"
#include "ProgramBase.h"
#include "CommandQueueBase.h"
#include "ContextBase.h"

#include <Methane/Instrumentation.h>

#include <taskflow/taskflow.hpp>
#include <fmt/format.h>

#include <string_view>

namespace Methane::Graphics
{

ParallelRenderCommandListBase::ParallelRenderCommandListBase(CommandQueueBase& command_queue, RenderPassBase& render_pass)
    : CommandListBase(command_queue, Type::ParallelRender)
    , m_render_pass_ptr(render_pass.GetPtr<RenderPassBase>())
{
    META_FUNCTION_TASK();
}

void ParallelRenderCommandListBase::SetValidationEnabled(bool is_validation_enabled)
{
    META_FUNCTION_TASK();
    m_is_validation_enabled = is_validation_enabled;
    for(const Ptr<RenderCommandListBase>& render_command_list_ptr : m_parallel_command_lists)
    {
        META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
        render_command_list_ptr->SetValidationEnabled(m_is_validation_enabled);
    }
}

void ParallelRenderCommandListBase::Reset(IDebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    ResetImpl(p_debug_group, [this, p_debug_group](const Data::Index command_list_index)
    {
        META_FUNCTION_TASK();
        const Ptr<RenderCommandListBase>& render_command_list_ptr = m_parallel_command_lists[command_list_index];
        META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
        render_command_list_ptr->Reset(p_debug_group ? p_debug_group->GetSubGroup(command_list_index) : nullptr);
    });
}

void ParallelRenderCommandListBase::ResetWithState(IRenderState& render_state, IDebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    ResetImpl(p_debug_group, [this, &render_state, p_debug_group](Data::Index command_list_index)
    {
        META_FUNCTION_TASK();
        const Ptr<RenderCommandListBase>& render_command_list_ptr = m_parallel_command_lists[command_list_index];
        META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
        render_command_list_ptr->ResetWithState(render_state, p_debug_group ? p_debug_group->GetSubGroup(command_list_index) : nullptr);
    });
}

template<typename ResetCommandListFn>
void ParallelRenderCommandListBase::ResetImpl(IDebugGroup* p_debug_group, const ResetCommandListFn& reset_command_list_fn) // NOSONAR - function can not be const
{
    CommandListBase::Reset();

    // Create per-thread debug sub-group:
    if (p_debug_group && !p_debug_group->HasSubGroups())
    {
        for(Data::Index render_command_list_index = 0; render_command_list_index < m_parallel_command_lists.size(); ++render_command_list_index)
        {
            p_debug_group->AddSubGroup(render_command_list_index, GetThreadCommandListName(p_debug_group->GetName(), render_command_list_index));
        }
    }

    // Per-thread render command lists can be reset in parallel only with DirectX 12 on Windows
#ifdef _WIN32
    tf::Taskflow reset_task_flow;
    reset_task_flow.for_each_index(0U, static_cast<uint32_t>(m_parallel_command_lists.size()), 1U, reset_command_list_fn);
    GetCommandQueueBase().GetContext().GetParallelExecutor().run(reset_task_flow).get();
#else
    for(Data::Index command_list_index = 0U; command_list_index < static_cast<Data::Index>(m_parallel_command_lists.size()); ++command_list_index)
        reset_command_list_fn(command_list_index);
#endif
}

void ParallelRenderCommandListBase::Commit()
{
    META_FUNCTION_TASK();
    tf::Taskflow commit_task_flow;
    commit_task_flow.for_each(m_parallel_command_lists.begin(), m_parallel_command_lists.end(),
        [](const Ptr<RenderCommandListBase>& render_command_list_ptr)
        {
            META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
            render_command_list_ptr->Commit();
        }
    );
    GetCommandQueueBase().GetContext().GetParallelExecutor().run(commit_task_flow).get();
    CommandListBase::Commit();
}

void ParallelRenderCommandListBase::SetViewState(IViewState& view_state)
{
    META_FUNCTION_TASK();
    for(const Ptr<RenderCommandListBase>& render_command_list_ptr : m_parallel_command_lists)
    {
        META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
        render_command_list_ptr->SetViewState(view_state);
    }
}

void ParallelRenderCommandListBase::SetParallelCommandListsCount(uint32_t count)
{
    META_FUNCTION_TASK();
    const auto initial_count = static_cast<uint32_t>(m_parallel_command_lists.size());
    if (count < initial_count)
    {
        m_parallel_command_lists.erase(m_parallel_command_lists.begin() + count);
        m_parallel_command_lists_refs.erase(m_parallel_command_lists_refs.begin() + count);
        return;
    }

    const std::string& name = GetName();
    m_parallel_command_lists.reserve(count);
    m_parallel_command_lists_refs.reserve(count);

    for(uint32_t cmd_list_index = initial_count; cmd_list_index < count; ++cmd_list_index)
    {
        m_parallel_command_lists.emplace_back(std::static_pointer_cast<RenderCommandListBase>(RenderCommandList::Create(*this)));
        m_parallel_command_lists.back()->SetValidationEnabled(m_is_validation_enabled);
        m_parallel_command_lists_refs.emplace_back(*m_parallel_command_lists.back());
        if (!name.empty())
        {
            m_parallel_command_lists.back()->SetName(GetThreadCommandListName(name, cmd_list_index));
        }
    }
}

void ParallelRenderCommandListBase::Execute(const ICommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    for(const Ptr<RenderCommandListBase>& render_command_list_ptr : m_parallel_command_lists)
    {
        META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
        render_command_list_ptr->Execute();
    }

    CommandListBase::Execute(completed_callback);
}

void ParallelRenderCommandListBase::Complete()
{
    META_FUNCTION_TASK();
    for(const Ptr<RenderCommandListBase>& render_command_list_ptr : m_parallel_command_lists)
    {
        META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
        render_command_list_ptr->Complete();
    }

    CommandListBase::Complete();
}

bool ParallelRenderCommandListBase::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!CommandListBase::SetName(name) || name.empty())
        return false;

    uint32_t render_cmd_list_index = 0;
    for(const Ptr<RenderCommandListBase>& render_cmd_list_ptr : m_parallel_command_lists)
    {
        META_CHECK_ARG_NOT_NULL(render_cmd_list_ptr);
        render_cmd_list_ptr->SetName(GetThreadCommandListName(name, render_cmd_list_index));
        render_cmd_list_index++;
    }
    return true;
}

RenderPassBase& ParallelRenderCommandListBase::GetPass()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_render_pass_ptr);
    return *m_render_pass_ptr;
}

std::string ParallelRenderCommandListBase::GetParallelCommandListDebugName(std::string_view base_name, std::string_view suffix)
{
    return base_name.empty() ? std::string() : fmt::format("{} {}", base_name, suffix);
}

std::string ParallelRenderCommandListBase::GetTrailingCommandListDebugName(std::string_view base_name, bool is_beginning)
{
    return GetParallelCommandListDebugName(base_name, is_beginning ? "[Beginning]" : "[Ending]");
}

std::string ParallelRenderCommandListBase::GetThreadCommandListName(std::string_view base_name, Data::Index index)
{
    return GetParallelCommandListDebugName(base_name, fmt::format("- Thread {}", index));
}

} // namespace Methane::Graphics
