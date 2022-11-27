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

FILE: Methane/Graphics/Base/CommandList.h
Base implementation of the command list interface.

******************************************************************************/

#pragma once

#include "Object.h"

#include <Methane/Graphics/RHI/IQueryPool.h>
#include <Methane/Graphics/RHI/IProgram.h>
#include <Methane/Graphics/RHI/ICommandList.h>
#include <Methane/Graphics/RHI/ICommandQueue.h>
#include <Methane/Data/Emitter.hpp>
#include <Methane/Memory.hpp>
#include <Methane/TracyGpu.hpp>
#include <Methane/Checks.hpp>

#include <stack>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace Methane::Graphics::Base
{

class CommandListDebugGroup
    : public Rhi::ICommandListDebugGroup
    , public Object
{
public:
    explicit CommandListDebugGroup(const std::string& name);

    // IObject overrides
    bool SetName(const std::string&) override;

    // IDebugGroup interface
    Rhi::ICommandListDebugGroup& AddSubGroup(Data::Index id, const std::string& name) final;
    Rhi::ICommandListDebugGroup* GetSubGroup(Data::Index id) const noexcept final;
    bool                         HasSubGroups() const noexcept final { return !m_sub_groups.empty(); }

private:
    Ptrs<Rhi::ICommandListDebugGroup> m_sub_groups;
};

class CommandQueue;
class ProgramBindings;

class CommandList // NOSONAR - custom destructor is used for logging, class has more than 35 methods
    : public Object
    , public virtual Rhi::ICommandList // NOSONAR
    , public Data::Emitter<Rhi::ICommandListCallback>
{
    friend class CommandQueue;

public:
    using DebugGroup = CommandListDebugGroup;

    struct CommandState final
    {
        // Raw pointer is used for program bindings instead of smart pointer for performance reasons
        // to get rid of shared_from_this() overhead required to acquire smart pointer from reference
        const ProgramBindings* program_bindings_ptr;
        Ptrs<Object>           retained_resources;
    };

    CommandList(CommandQueue& command_queue, Type type);
    ~CommandList() override;

    // ICommandList interface
    Type  GetType() const noexcept override                         { return m_type; }
    State GetState() const noexcept override                        { return m_state; }
    void  PushDebugGroup(IDebugGroup& debug_group) override;
    void  PopDebugGroup() override;
    void  Reset(IDebugGroup* p_debug_group = nullptr) override;
    void  ResetOnce(IDebugGroup* p_debug_group = nullptr) final;
    void  SetProgramBindings(Rhi::IProgramBindings& program_bindings, Rhi::ProgramBindingsApplyBehaviorMask apply_behavior) override;
    void  Commit() override;
    void  WaitUntilCompleted(uint32_t timeout_ms = 0U) override;
    Data::TimeRange GetGpuTimeRange(bool in_cpu_nanoseconds) const override;
    Rhi::ICommandQueue& GetCommandQueue() final;

    // CommandList interface
    virtual void Execute(const CompletedCallback& completed_callback = {});
    virtual void Complete(); // Called from command queue thread, which is tracking GPU execution

    DebugGroup* GetTopOpenDebugGroup() const;
    void PushOpenDebugGroup(IDebugGroup& debug_group);
    void ClearOpenDebugGroups();

    CommandQueue&          GetBaseCommandQueue();
    const CommandQueue&    GetBaseCommandQueue() const;
    const ProgramBindings* GetProgramBindingsPtr() const noexcept { return GetCommandState().program_bindings_ptr; }
    Ptr<CommandList>       GetCommandListPtr()                    { return GetPtr<CommandList>(); }

    inline void RetainResource(const Ptr<Object>& resource_ptr)   { if (resource_ptr) m_command_state.retained_resources.emplace_back(resource_ptr); }
    inline void RetainResource(Object& resource)                  { m_command_state.retained_resources.emplace_back(resource.GetBasePtr()); }
    inline void ReleaseRetainedResources()                        { m_command_state.retained_resources.clear(); }

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<Object, T>>>
    inline void RetainResources(const Ptrs<T>& resource_ptrs)
    {
        for(const Ptr<T>& resource_ptr : resource_ptrs)
            RetainResource(std::static_pointer_cast<Object>(resource_ptr));
    }

protected:
    virtual void ResetCommandState();
    virtual void ApplyProgramBindings(ProgramBindings& program_bindings, Rhi::ProgramBindingsApplyBehaviorMask apply_behavior);

    CommandState&       GetCommandState()               { return m_command_state; }
    const CommandState& GetCommandState() const         { return m_command_state; }

    void SetCommandListState(State state);
    void SetCommandListStateNoLock(State state);
    bool IsExecutingOnAnyFrame() const           { return m_state == State::Executing; }
    bool IsCommitted() const                     { return m_state == State::Committed; }
    bool IsExecuting() const                     { return m_state == State::Executing; }
    auto LockStateMutex() const                  { return std::scoped_lock(m_state_mutex); }

    void InitializeTimestampQueries();
    void BeginGpuZone();
    void EndGpuZone();

    void VerifyEncodingState() const;

private:
    using DebugGroupStack  = std::stack<Ptr<DebugGroup>>;

    void CompleteInternal();

    const Type        m_type;
    Ptr<CommandQueue> m_command_queue_ptr;
    CommandState      m_command_state;
    DebugGroupStack   m_open_debug_groups;
    CompletedCallback m_completed_callback;
    State             m_state = State::Pending;

    mutable TracyLockable(std::recursive_mutex, m_state_mutex)
    TracyLockable(std::mutex,   m_state_change_mutex)
    std::condition_variable_any m_state_change_condition_var;
    TRACY_GPU_SCOPE_TYPE        m_tracy_gpu_scope;

#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
    Ptr<Rhi::ITimestampQuery> m_begin_timestamp_query_ptr;
    Ptr<Rhi::ITimestampQuery> m_end_timestamp_query_ptr;
#endif
};

class CommandListSet
    : public Rhi::ICommandListSet
    , protected Data::Receiver<Rhi::IObjectCallback>
    , public std::enable_shared_from_this<CommandListSet>
{
public:
    explicit CommandListSet(const Refs<Rhi::ICommandList>& command_list_refs, Opt<Data::Index> frame_index_opt);

    // ICommandListSet overrides
    Data::Size                     GetCount() const noexcept final       { return static_cast<Data::Size>(m_refs.size()); }
    const Refs<Rhi::ICommandList>& GetRefs() const noexcept final        { return m_refs; }
    Rhi::ICommandList&             operator[](Data::Index index) const final;
    const Opt<Data::Index>&  GetFrameIndex() const noexcept final        { return m_frame_index_opt; }

    // CommandListSet interface
    virtual void Execute(const Rhi::ICommandList::CompletedCallback& completed_callback);
    virtual void WaitUntilCompleted() = 0;

    bool IsExecuting() const noexcept { return m_is_executing; }
    void Complete() const;

    [[nodiscard]] Ptr<CommandListSet>      GetPtr()                     { return shared_from_this(); }
    [[nodiscard]] const Refs<CommandList>& GetBaseRefs() const noexcept { return m_base_refs; }
    [[nodiscard]] const CommandList&       GetBaseCommandList(Data::Index index) const;
    [[nodiscard]] CommandQueue&            GetBaseCommandQueue()        { return m_base_refs.back().get().GetBaseCommandQueue(); }
    [[nodiscard]] const CommandQueue&      GetBaseCommandQueue() const  { return m_base_refs.back().get().GetBaseCommandQueue(); }
    [[nodiscard]] const std::string&       GetCombinedName();

protected:
    // IObjectCallback interface
    void OnObjectNameChanged(Rhi::IObject&, const std::string&) override;

private:
    Refs<Rhi::ICommandList> m_refs;
    Refs<CommandList>       m_base_refs;
    Ptrs<CommandList>       m_base_ptrs;
    Opt<Data::Index>        m_frame_index_opt;
    std::string             m_combined_name;

    mutable TracyLockable(std::mutex, m_command_lists_mutex)
    mutable std::atomic<bool> m_is_executing = false;
};

} // namespace Methane::Graphics::Base
