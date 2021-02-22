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
#include <Methane/Data/Math.hpp>

#include <taskflow/taskflow.hpp>
#include <fmt/format.h>

#include <string_view>

namespace Methane::Graphics
{

inline std::string GetThreadCommandListName(std::string_view name, Data::Index index)
{
    return fmt::format("{} [Thread {}", name, index);
}

ParallelRenderCommandListBase::ParallelRenderCommandListBase(CommandQueueBase& command_queue, RenderPassBase& render_pass)
    : CommandListBase(command_queue, Type::ParallelRender)
    , m_render_pass_ptr(render_pass.GetRenderPassPtr())
{
    META_FUNCTION_TASK();
}

void ParallelRenderCommandListBase::SetValidationEnabled(bool is_validation_enabled)
{
    META_FUNCTION_TASK();
    m_is_validation_enabled = is_validation_enabled;
    for(const Ptr<RenderCommandList>& render_command_list_ptr : m_parallel_command_lists)
    {
        META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
        render_command_list_ptr->SetValidationEnabled(m_is_validation_enabled);
    }
}

void ParallelRenderCommandListBase::Reset(DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    ResetImpl(p_debug_group, [this, p_debug_group](size_t command_list_index)
    {
        META_FUNCTION_TASK();
        const Ptr<RenderCommandList>& render_command_list_ptr = m_parallel_command_lists[command_list_index];
        META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
        render_command_list_ptr->Reset(p_debug_group ? p_debug_group->GetSubGroup(static_cast<Data::Index>(command_list_index)) : nullptr);
    });
}

void ParallelRenderCommandListBase::ResetWithState(RenderState& render_state, DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    ResetImpl(p_debug_group, [this, &render_state, p_debug_group](size_t command_list_index)
    {
        META_FUNCTION_TASK();
        const Ptr<RenderCommandList>& render_command_list_ptr = m_parallel_command_lists[command_list_index];
        META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
        render_command_list_ptr->ResetWithState(render_state, p_debug_group ? p_debug_group->GetSubGroup(static_cast<Data::Index>(command_list_index)) : nullptr);
    });
}

template<typename ResetCommandListFn>
void ParallelRenderCommandListBase::ResetImpl(DebugGroup* p_debug_group, const ResetCommandListFn& reset_command_list_fn) // NOSONAR - function can not be const
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
    reset_task_flow.for_each_index_guided(0, static_cast<int>(m_parallel_command_lists.size()), 1, reset_command_list_fn,
                                          Data::GetParallelChunkSizeAsInt(m_parallel_command_lists.size()));
    GetCommandQueueBase().GetContext().GetParallelExecutor().run(reset_task_flow).get();
#else
    for(size_t command_list_index = 0U; command_list_index < m_parallel_command_lists.size(); ++command_list_index)
        reset_command_list_fn(command_list_index);
#endif
}

void ParallelRenderCommandListBase::Commit()
{
    META_FUNCTION_TASK();
    for(const Ptr<RenderCommandList>& render_command_list_ptr : m_parallel_command_lists)
    {
        META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
        render_command_list_ptr->Commit();
    }

    CommandListBase::Commit();
}

void ParallelRenderCommandListBase::SetViewState(ViewState& view_state)
{
    META_FUNCTION_TASK();
    for(const Ptr<RenderCommandList>& render_command_list_ptr : m_parallel_command_lists)
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
        m_parallel_command_lists.resize(count);
        return;
    }

    const std::string& name = GetName();
    m_parallel_command_lists.reserve(count);
    for(uint32_t cmd_list_index = initial_count; cmd_list_index < count; ++cmd_list_index)
    {
        m_parallel_command_lists.emplace_back(RenderCommandList::Create(*this));
        if (!name.empty())
        {
            m_parallel_command_lists.back()->SetName(GetThreadCommandListName(name, cmd_list_index));
        }
    }
}

void ParallelRenderCommandListBase::Execute(uint32_t frame_index, const CommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    for(const Ptr<RenderCommandList>& render_command_list_ptr : m_parallel_command_lists)
    {
        META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
        auto& thread_render_command_list = static_cast<RenderCommandListBase&>(*render_command_list_ptr);
        thread_render_command_list.Execute(frame_index);
    }

    CommandListBase::Execute(frame_index, completed_callback);
}

void ParallelRenderCommandListBase::Complete(uint32_t frame_index)
{
    META_FUNCTION_TASK();
    for(const Ptr<RenderCommandList>& render_command_list_ptr : m_parallel_command_lists)
    {
        META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
        auto& thread_render_command_list = static_cast<RenderCommandListBase&>(*render_command_list_ptr);
        thread_render_command_list.Complete(frame_index);
    }

    CommandListBase::Complete(frame_index);
}

void ParallelRenderCommandListBase::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    CommandListBase::SetName(name);

    if (name.empty())
        return;

    uint32_t render_cmd_list_index = 0;
    for(const Ptr<RenderCommandList>& render_cmd_list_ptr : m_parallel_command_lists)
    {
        META_CHECK_ARG_NOT_NULL(render_cmd_list_ptr);
        render_cmd_list_ptr->SetName(GetThreadCommandListName(name, render_cmd_list_index));
        render_cmd_list_index++;
    }
}

RenderPassBase& ParallelRenderCommandListBase::GetPass()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_render_pass_ptr);
    return static_cast<RenderPassBase&>(*m_render_pass_ptr);
}

} // namespace Methane::Graphics
