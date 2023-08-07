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

FILE: Methane/Graphics/Base/CommandList.cpp
Base implementation of the command list interface.

******************************************************************************/

#include <Methane/Graphics/Base/CommandList.h>
#include <Methane/Graphics/Base/CommandListDebugGroup.h>
#include <Methane/Graphics/Base/Device.h>
#include <Methane/Graphics/Base/CommandQueue.h>
#include <Methane/Graphics/Base/ProgramBindings.h>
#include <Methane/Graphics/Base/Resource.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>

// Disable debug groups instrumentation with discontinuous CPU frames in Tracy,
// because it is not working for parallel render command lists by some reason
//#define METHANE_DEBUG_GROUP_FRAMES_ENABLED

namespace Methane::Graphics::Base
{

#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
static Data::TimeRange GetNormalTimeRange(Timestamp start, Timestamp end)
{
    return Data::TimeRange(std::min(start, end), std::max(start, end));
}
#endif

CommandList::CommandList(CommandQueue& command_queue, Type type)
    : m_type(type)
    , m_command_queue_ptr(command_queue.GetPtr<CommandQueue>())
    , m_tracy_gpu_scope(TRACY_GPU_SCOPE_INIT(command_queue.GetTracyContextPtr())) // NOSONAR - do not use in-class initializer
{
    META_FUNCTION_TASK();
    TRACY_GPU_SCOPE_TRY_BEGIN_UNNAMED(m_tracy_gpu_scope);
    META_LOG("{} Command list '{}' was created", magic_enum::enum_name(m_type), GetName());
    META_UNUSED(m_tracy_gpu_scope); // silence unused member warning on MacOS when Tracy GPU profiling
}

CommandList::~CommandList()
{
    META_FUNCTION_TASK();
    META_LOG("{} Command list '{}' was destroyed", magic_enum::enum_name(m_type), GetName());
}

void CommandList::PushDebugGroup(IDebugGroup& debug_group)
{
    META_FUNCTION_TASK();
    VerifyEncodingState();

#ifdef METHANE_DEBUG_GROUP_FRAMES_ENABLED
    META_CPU_FRAME_START(debug_group.GetName().data());
#endif
    META_LOG("{} Command list '{}' PUSH debug group '{}'", magic_enum::enum_name(m_type), GetName(), debug_group.GetName());

    PushOpenDebugGroup(debug_group);
}

void CommandList::PopDebugGroup()
{
    META_FUNCTION_TASK();
    if (m_open_debug_groups.empty())
    {
        throw std::underflow_error("Can not pop debug group, since no debug groups were pushed");
    }

    META_LOG("{} Command list '{}' POP debug group '{}'", magic_enum::enum_name(m_type), GetName(), GetTopOpenDebugGroup()->GetName());
#ifdef METHANE_DEBUG_GROUP_FRAMES_ENABLED
    META_CPU_FRAME_END(GetTopOpenDebugGroup()->GetName().data());
#endif

    m_open_debug_groups.pop();
}

void CommandList::Reset(IDebugGroup* debug_group_ptr)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_state_mutex);

    META_CHECK_ARG_DESCR(m_state, m_state != State::Committed && m_state != State::Executing, "can not reset command list in committed or executing state");
    META_LOG("{} Command list '{}' RESET commands encoding{}", magic_enum::enum_name(m_type), GetName(),
             debug_group_ptr ? fmt::format("with debug group '{}'", debug_group_ptr->GetName()) : "");

    ResetCommandState();
    SetCommandListStateNoLock(State::Encoding);

    const bool debug_group_changed = GetTopOpenDebugGroup() != debug_group_ptr;
    if (!m_open_debug_groups.empty() && debug_group_changed)
    {
        PopDebugGroup();
    }

    TRACY_GPU_SCOPE_TRY_BEGIN_NAMED(m_tracy_gpu_scope, GetName());

    if (debug_group_ptr && debug_group_changed)
    {
        PushDebugGroup(*debug_group_ptr);
    }
}

void CommandList::ResetOnce(IDebugGroup* debug_group_ptr)
{
    META_FUNCTION_TASK();
    if (m_state == State::Encoding)
    {
        META_LOG("{} Command list '{}' was already RESET", magic_enum::enum_name(GetType()), GetName());
        return;
    }

    Reset(debug_group_ptr);
}

void CommandList::SetProgramBindings(Rhi::IProgramBindings& program_bindings, Rhi::ProgramBindingsApplyBehaviorMask apply_behavior)
{
    META_FUNCTION_TASK();
    if (m_command_state.program_bindings_ptr == std::addressof(program_bindings))
        return;

    META_LOG("{} Command list '{}' SET PROGRAM BINDINGS for program '{}':\n{}",
             magic_enum::enum_name(GetType()), GetName(), program_bindings.GetProgram().GetName(),
             static_cast<std::string>(program_bindings));

    auto& program_bindings_base = static_cast<ProgramBindings&>(program_bindings);
    ApplyProgramBindings(program_bindings_base, apply_behavior);

    if (constexpr Rhi::ProgramBindingsApplyBehaviorMask constant_once_and_changes_only({
            Rhi::ProgramBindingsApplyBehavior::ConstantOnce,
            Rhi::ProgramBindingsApplyBehavior::ChangesOnly
        });
        apply_behavior.HasAnyBits(constant_once_and_changes_only))
    {
        m_command_state.program_bindings_ptr = std::addressof(program_bindings_base);
    }

    if (apply_behavior.HasAnyBit(Rhi::ProgramBindingsApplyBehavior::RetainResources))
    {
        RetainResource(program_bindings_base.GetBasePtr());
    }
}

void CommandList::Commit()
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_state_mutex);

    META_CHECK_ARG_EQUAL_DESCR(m_state, State::Encoding,
                               "{} command list '{}' in {} state can not be committed; only command lists in 'Encoding' state can be committed",
                               magic_enum::enum_name(m_type), GetName(), magic_enum::enum_name(m_state));

    TRACY_GPU_SCOPE_END(m_tracy_gpu_scope);
    META_LOG("{} Command list '{}' COMMIT", magic_enum::enum_name(m_type), GetName());

    SetCommandListStateNoLock(State::Committed);
    
    while (!m_open_debug_groups.empty())
    {
        PopDebugGroup();
    }
}

void CommandList::WaitUntilCompleted(uint32_t timeout_ms)
{
    META_FUNCTION_TASK();
    std::unique_lock pending_state_lock(m_state_change_mutex);
    const auto is_completed = [this] { return m_state != State::Executing; };
    if (is_completed())
        return;

    META_LOG("{} Command list '{}' WAITING for completion", magic_enum::enum_name(m_type), GetName());

    if (timeout_ms == 0U)
    {
        m_state_change_condition_var.wait(pending_state_lock, is_completed);
    }
    else
    {
        m_state_change_condition_var.wait_for(pending_state_lock, std::chrono::milliseconds(timeout_ms), is_completed); // NOSONAR - false positive
    }
}

void CommandList::Execute(const CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_state_mutex);

    META_CHECK_ARG_EQUAL_DESCR(m_state, State::Committed,
                               "{} command list '{}' in {} state can not be executed; only command lists in 'Committed' state can be executed",
                               magic_enum::enum_name(m_type), GetName(), magic_enum::enum_name(m_state));

    META_LOG("{} Command list '{}' EXECUTE", magic_enum::enum_name(m_type), GetName());

    m_completed_callback = completed_callback;

    SetCommandListStateNoLock(State::Executing);
}

void CommandList::Complete()
{
    META_FUNCTION_TASK();
    CompleteInternal();

    if (m_completed_callback)
        m_completed_callback(*this);
    
    Data::Emitter<Rhi::ICommandListCallback>::Emit(&Rhi::ICommandListCallback::OnCommandListExecutionCompleted, *this);
}

void CommandList::CompleteInternal()
{
    std::scoped_lock lock_guard(m_state_mutex);

    META_CHECK_ARG_EQUAL_DESCR(m_state, State::Executing,
                               "{} command list '{}' in {} state can not be completed; only command lists in 'Executing' state can be completed",
                               magic_enum::enum_name(m_type), GetName(), magic_enum::enum_name(m_state));

    ReleaseRetainedResources();
    SetCommandListStateNoLock(State::Pending);

    TRACY_GPU_SCOPE_COMPLETE(m_tracy_gpu_scope, GetGpuTimeRange(false));
    META_LOG("{} Command list '{}' was COMPLETED with GPU timings {}", magic_enum::enum_name(m_type), GetName(), static_cast<std::string>(GetGpuTimeRange(true)));
}

CommandListDebugGroup* CommandList::GetTopOpenDebugGroup() const
{
    META_FUNCTION_TASK();
    return m_open_debug_groups.empty() ? nullptr : m_open_debug_groups.top().get();
}

void CommandList::PushOpenDebugGroup(IDebugGroup& debug_group)
{
    META_FUNCTION_TASK();
    m_open_debug_groups.emplace(static_cast<DebugGroup&>(debug_group).GetPtr<DebugGroup>());
}

void CommandList::ClearOpenDebugGroups()
{
    META_FUNCTION_TASK();
    while(!m_open_debug_groups.empty())
    {
        m_open_debug_groups.pop();
    }
}

void CommandList::SetCommandListState(State state)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_state_mutex);
    SetCommandListStateNoLock(state);
}

void CommandList::SetCommandListStateNoLock(State state)
{
    META_FUNCTION_TASK();
    if (m_state == state)
        return;

    META_LOG("{} Command list '{}' change state from {} to {}",
             magic_enum::enum_name(m_type), GetName(), magic_enum::enum_name(m_state), magic_enum::enum_name(state));

    m_state = state;
    m_state_change_condition_var.notify_one();
    
    Data::Emitter<Rhi::ICommandListCallback>::Emit(&Rhi::ICommandListCallback::OnCommandListStateChanged, *this);
}

void CommandList::VerifyEncodingState() const
{
    META_CHECK_ARG_EQUAL_DESCR(m_state, State::Encoding,
                               "{} command list '{}' encoding is not possible in '{}' state",
                               magic_enum::enum_name(m_type), GetName(), magic_enum::enum_name(m_state));
}

void CommandList::InitializeTimestampQueries() // NOSONAR - function is not const when instrumentation enabled
{
#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
    META_FUNCTION_TASK();
    Rhi::ITimestampQueryPool* query_pool_ptr = GetCommandQueue().GetTimestampQueryPoolPtr().get();
    // In DirectX copy command queue may have no support of timestamp queries
    if (!query_pool_ptr)
        return;

    m_begin_timestamp_query_ptr = query_pool_ptr->CreateTimestampQuery(*this);
    m_end_timestamp_query_ptr   = query_pool_ptr->CreateTimestampQuery(*this);
#endif
}

void CommandList::BeginGpuZone() // NOSONAR - function is not const when instrumentation enabled
{
#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
    META_FUNCTION_TASK();
    // Insert beginning GPU timestamp query
    if (m_begin_timestamp_query_ptr)
        m_begin_timestamp_query_ptr->InsertTimestamp();
#endif
}

void CommandList::EndGpuZone() // NOSONAR - function is not const when instrumentation enabled
{
#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
    META_FUNCTION_TASK();
    // Insert ending GPU timestamp query
    // and resolve timestamps of beginning and ending queries
    if (m_end_timestamp_query_ptr)
    {
        m_end_timestamp_query_ptr->InsertTimestamp();
        m_end_timestamp_query_ptr->ResolveTimestamp();
    }
    if (m_begin_timestamp_query_ptr)
    {
        m_begin_timestamp_query_ptr->ResolveTimestamp();
    }
#endif
}

Data::TimeRange CommandList::GetGpuTimeRange(bool in_cpu_nanoseconds) const
{
    META_FUNCTION_TASK();
#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
    if (m_begin_timestamp_query_ptr && m_end_timestamp_query_ptr)
    {
        META_CHECK_ARG_EQUAL_DESCR(GetState(), CommandList::State::Pending, "can not get GPU time range of encoding, executing or not committed command list");
        return in_cpu_nanoseconds
             ? GetNormalTimeRange(m_begin_timestamp_query_ptr->GetCpuNanoseconds(), m_end_timestamp_query_ptr->GetCpuNanoseconds())
             : GetNormalTimeRange(m_begin_timestamp_query_ptr->GetGpuTimestamp(),   m_end_timestamp_query_ptr->GetGpuTimestamp());
    }
#else
    META_UNUSED(in_cpu_nanoseconds);
#endif
    return { 0U, 0U };
}

Rhi::ICommandQueue& CommandList::GetCommandQueue()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_command_queue_ptr);
    return *m_command_queue_ptr;
}

void CommandList::ResetCommandState()
{
    META_FUNCTION_TASK();
    m_command_state.program_bindings_ptr = nullptr;
}

void CommandList::ApplyProgramBindings(ProgramBindings& program_bindings, Rhi::ProgramBindingsApplyBehaviorMask apply_behavior)
{
    program_bindings.Apply(*this, apply_behavior);
}

CommandQueue& CommandList::GetBaseCommandQueue()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_command_queue_ptr);
    return *m_command_queue_ptr;
}

const CommandQueue& CommandList::GetBaseCommandQueue() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_command_queue_ptr);
    return *m_command_queue_ptr;
}

} // namespace Methane::Graphics::Base
