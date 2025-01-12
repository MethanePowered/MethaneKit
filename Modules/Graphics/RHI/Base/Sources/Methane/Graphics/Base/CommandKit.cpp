/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Base/CommandKit.h
Methane command kit implementation.

******************************************************************************/

#include <Methane/Graphics/Base/CommandKit.h>
#include <Methane/Graphics/Base/CommandListSet.h>
#include <Methane/Graphics/Base/RenderCommandList.h>
#include <Methane/Graphics/Base/ComputeCommandList.h>
#include <Methane/Graphics/Base/CommandQueue.h>

#include <Methane/Graphics/RHI/IFence.h>
#include <Methane/Graphics/RHI/IContext.h>
#include <Methane/Graphics/RHI/ICommandKit.h>
#include <Methane/Graphics/RHI/ICommandListDebugGroup.h>
#include <Methane/Graphics/RHI/ITransferCommandList.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <atomic>
#include <condition_variable>

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

namespace Methane::Graphics::Base
{

static constexpr uint32_t g_max_cmd_lists_count = 32U;

static std::string GetCommandListNameById(Rhi::CommandListId id)
{
    META_FUNCTION_TASK();
    switch(static_cast<Rhi::CommandListPurpose>(id))
    {
    using enum Rhi::CommandListPurpose;
    case Default:        return "Default";
    case PreUploadSync:  return "PreUploadSync";
    case PostUploadSync: return "PostUploadSync";
    }
    return std::to_string(id);
}

CommandKit::CommandKit(const Rhi::IContext& context, Rhi::CommandListType cmd_list_type)
    : m_context(context)
    , m_cmd_list_type(cmd_list_type)
{
    META_FUNCTION_TASK();
    if (context.GetType() == Rhi::ContextType::Compute)
    {
        META_CHECK_NOT_EQUAL_DESCR(cmd_list_type, Rhi::CommandListType::Render,
                                   "compute context can not be used to create render command queues");
    }
    META_CHECK_NOT_EQUAL_DESCR(cmd_list_type, Rhi::CommandListType::ParallelRender,
                               "command queue should be created with Render type to support ParallelRender command lists");
}

CommandKit::CommandKit(Rhi::ICommandQueue& cmd_queue)
    : Object(cmd_queue.GetName())
    , m_context(cmd_queue.GetContext())
    , m_cmd_list_type(cmd_queue.GetCommandListType())
    , m_cmd_queue_ptr(static_cast<CommandQueue&>(cmd_queue).GetPtr<CommandQueue>())
{
}

bool CommandKit::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Object::SetName(name))
        return false;

    if (m_cmd_queue_ptr)
        m_cmd_queue_ptr->SetName(fmt::format("{} Command Queue", GetName()));

    for(size_t cmd_list_index = 0; cmd_list_index < m_cmd_list_ptrs.size(); ++cmd_list_index)
    {
        const Ptr<Rhi::ICommandList>& cmd_list_ptr = m_cmd_list_ptrs[cmd_list_index];
        if (cmd_list_ptr)
            cmd_list_ptr->SetName(fmt::format("{} Command List {}", GetName(), cmd_list_index));
    }

    for(size_t fence_index = 0; fence_index < m_fence_ptrs.size(); ++fence_index)
    {
        const Ptr<Rhi::IFence>& fence_ptr = m_fence_ptrs[fence_index];
        if (fence_ptr)
            fence_ptr->SetName(fmt::format("{} Fence {}", GetName(), fence_index));
    }

    return true;
}

Rhi::ICommandQueue& CommandKit::GetQueue() const
{
    META_FUNCTION_TASK();
    if (m_cmd_queue_ptr)
        return *m_cmd_queue_ptr;

    m_cmd_queue_ptr = Rhi::ICommandQueue::Create(m_context, m_cmd_list_type);
    m_cmd_queue_ptr->SetName(fmt::format("{} Command Queue", GetName()));
    return *m_cmd_queue_ptr;
}

bool CommandKit::HasList(Rhi::CommandListId cmd_list_id) const noexcept
{
    META_FUNCTION_TASK();
    const CommandListIndex cmd_list_index = GetCommandListIndexById(cmd_list_id);
    return cmd_list_index < m_cmd_list_ptrs.size() && m_cmd_list_ptrs[cmd_list_index];
}

bool CommandKit::HasListWithState(Rhi::CommandListState cmd_list_state, Rhi::CommandListId cmd_list_id) const noexcept
{
    META_FUNCTION_TASK();
    const CommandListIndex cmd_list_index = GetCommandListIndexById(cmd_list_id);
    return cmd_list_index < m_cmd_list_ptrs.size() && m_cmd_list_ptrs[cmd_list_index] && m_cmd_list_ptrs[cmd_list_index]->GetState() == cmd_list_state;
}

Rhi::ICommandList& CommandKit::GetList(Rhi::CommandListId cmd_list_id = 0U) const
{
    META_FUNCTION_TASK();
    const CommandListIndex cmd_list_index = GetCommandListIndexById(cmd_list_id);
    META_CHECK_LESS_DESCR(cmd_list_index, g_max_cmd_lists_count, "no more than 32 command lists are supported in one command kit");
    if (cmd_list_index >= m_cmd_list_ptrs.size())
        m_cmd_list_ptrs.resize(cmd_list_index + 1);

    Ptr<Rhi::ICommandList>& cmd_list_ptr = m_cmd_list_ptrs[cmd_list_index];
    if (cmd_list_ptr)
        return *cmd_list_ptr;

    switch (m_cmd_list_type)
    {
    case Rhi::CommandListType::Transfer: cmd_list_ptr = GetQueue().CreateTransferCommandList(); break;
    case Rhi::CommandListType::Render:   cmd_list_ptr = RenderCommandList::CreateForSynchronization(GetQueue()); break;
    case Rhi::CommandListType::Compute:  cmd_list_ptr = GetQueue().CreateComputeCommandList(); break;
    default: META_UNEXPECTED(m_cmd_list_type);
    }

    cmd_list_ptr->SetName(fmt::format("{} Helper List {}", GetName(), GetCommandListNameById(cmd_list_id)));
    return *cmd_list_ptr;
}

Rhi::ICommandList& CommandKit::GetListForEncoding(Rhi::CommandListId cmd_list_id, std::string_view debug_group_name) const
{
    META_FUNCTION_TASK();
    Rhi::ICommandList& cmd_list = GetList(cmd_list_id);

    // FIXME: loop with wait timeout iterations is used to workaround sporadic deadlock on command list wait for completion
    //        reproduced at high rate of resource updates (in typography tutorial)
    while(cmd_list.GetState() == Rhi::CommandListState::Executing)
        cmd_list.WaitUntilCompleted(16);

    if (cmd_list.GetState() == Rhi::CommandListState::Pending)
    {
        if (debug_group_name.empty())
        {
            cmd_list.Reset();
        }
        else
        {
            META_DEBUG_GROUP_CREATE_VAR(s_debug_region_name, std::string(debug_group_name));
            cmd_list.Reset(s_debug_region_name.get());
        }
    }

    return cmd_list;
}

Rhi::ICommandListSet& CommandKit::GetListSet(Rhi::CommandListIdSpan cmd_list_ids, Opt<Data::Index> frame_index_opt) const
{
    META_FUNCTION_TASK();
    META_CHECK_NOT_EMPTY(cmd_list_ids);
    const CommandListSetId cmd_list_set_id = GetCommandListSetId(cmd_list_ids, frame_index_opt);

    Ptr<Rhi::ICommandListSet>& cmd_list_set_ptr = m_cmd_list_set_by_id[cmd_list_set_id];
    if (cmd_list_set_ptr && cmd_list_set_ptr->GetCount() == cmd_list_ids.size())
        return *cmd_list_set_ptr;

    Refs<Rhi::ICommandList> command_list_refs;
    for(Rhi::CommandListId cmd_list_id : cmd_list_ids)
    {
        command_list_refs.emplace_back(GetList(cmd_list_id));
    }

    cmd_list_set_ptr = Rhi::ICommandListSet::Create(command_list_refs, frame_index_opt);
    return *cmd_list_set_ptr;
}

Rhi::IFence& CommandKit::GetFence(Rhi::CommandListId fence_id) const
{
    META_FUNCTION_TASK();
    const uint32_t fence_index = GetCommandListIndexById(fence_id);
    if (fence_index >= m_fence_ptrs.size())
        m_fence_ptrs.resize(fence_index + 1);

    Ptr<Rhi::IFence>& fence_ptr = m_fence_ptrs[fence_index];

    if (fence_ptr)
        return *fence_ptr;

    fence_ptr = Rhi::IFence::Create(GetQueue());
    fence_ptr->SetName(fmt::format("{} Fence {}", GetName(), fence_id));
    return *fence_ptr;
}

void CommandKit::ExecuteListSet(Rhi::CommandListIdSpan cmd_list_ids, Opt<Data::Index> frame_index_opt) const
{
    META_FUNCTION_TASK();
    GetQueue().Execute(GetListSet(cmd_list_ids, frame_index_opt));
}

void CommandKit::ExecuteListSetAndWaitForCompletion(Rhi::CommandListIdSpan cmd_list_ids, Opt<Data::Index> frame_index_opt) const
{
    META_FUNCTION_TASK();
    std::mutex                  execution_wait_mutex;
    size_t                      executing_cmd_list_count = cmd_list_ids.size();
    std::condition_variable_any executing_cmd_list_set_condition_var;
    GetQueue().Execute(GetListSet(cmd_list_ids, frame_index_opt),
                       [&executing_cmd_list_count, &execution_wait_mutex, &executing_cmd_list_set_condition_var](const Rhi::ICommandList&)
                       {
                           std::lock_guard lock(execution_wait_mutex);
                           executing_cmd_list_count--;
                           executing_cmd_list_set_condition_var.notify_one();
                       });

    std::unique_lock lock(execution_wait_mutex);
    executing_cmd_list_set_condition_var.wait(lock, [&executing_cmd_list_count]() { return executing_cmd_list_count == 0U; });
}

CommandKit::CommandListIndex CommandKit::GetCommandListIndexById(Rhi::CommandListId cmd_list_id) const noexcept
{
    META_FUNCTION_TASK();
    const auto [it, success] = m_cmd_list_index_by_id.try_emplace(cmd_list_id, static_cast<CommandListIndex>(m_cmd_list_index_by_id.size()));
    return it->second;
}

CommandKit::CommandListSetId CommandKit::GetCommandListSetId(Rhi::CommandListIdSpan cmd_list_ids, Opt<Data::Index> frame_index_opt) const
{
    META_FUNCTION_TASK();
    META_CHECK_LESS_DESCR(cmd_list_ids.size(), g_max_cmd_lists_count, "too many command lists in a set");
    uint32_t set_id = 0;
    for(const uint32_t cmd_list_id : cmd_list_ids)
    {
        const uint32_t cmd_list_index = GetCommandListIndexById(cmd_list_id);
        META_CHECK_LESS_DESCR(cmd_list_index, g_max_cmd_lists_count, "no more than 32 command lists are supported in one command kit");
        set_id += 1 << cmd_list_index;
    }
    return CommandListSetId(frame_index_opt, set_id);
}

} // namespace Methane::Graphics::Base
