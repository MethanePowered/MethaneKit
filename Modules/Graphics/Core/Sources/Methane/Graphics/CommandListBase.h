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

FILE: Methane/Graphics/CommandListBase.h
Base implementation of the command list interface.

******************************************************************************/

#pragma once

#include "ObjectBase.h"

#include <Methane/Graphics/Program.h>
#include <Methane/Graphics/CommandList.h>
#include <Methane/Graphics/CommandQueue.h>
#include <Methane/Data/Emitter.hpp>
#include <Methane/Memory.hpp>
#include <Methane/TracyGpu.hpp>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>
#include <stack>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace Methane::Graphics
{

class CommandQueueBase;
class ProgramBindingsBase;

class CommandListBase
    : public ObjectBase
    , public virtual CommandList // NOSONAR
    , public Data::Emitter<ICommandListCallback>
{
    friend class CommandQueueBase;

public:
    struct CommandState final
    {
        // Raw pointer is used for program bindings instead of smart pointer for performance reasons
        // to get rid of shared_from_this() overhead required to acquire smart pointer from reference
        const ProgramBindingsBase* program_bindings_ptr;
        Ptrs<ObjectBase>           retained_resources;
    };

    class DebugGroupBase
        : public DebugGroup
        , public ObjectBase
    {
    public:
        explicit DebugGroupBase(const std::string& name);

        // Object overrides
        bool SetName(const std::string&) override;

        // DebugGroup interface
        DebugGroup& AddSubGroup(Data::Index id, const std::string& name) final;
        DebugGroup* GetSubGroup(Data::Index id) const noexcept final;
        bool        HasSubGroups() const noexcept final { return !m_sub_groups.empty(); }

    private:
        Ptrs<DebugGroup> m_sub_groups;
    };

    CommandListBase(CommandQueueBase& command_queue, Type type);
    ~CommandListBase() override;

    // CommandList interface
    Type  GetType() const noexcept override                         { return m_type; }
    State GetState() const noexcept override                        { return m_state; }
    void  PushDebugGroup(DebugGroup& debug_group) override;
    void  PopDebugGroup() override;
    void  Reset(DebugGroup* p_debug_group = nullptr) override;
    void  ResetOnce(DebugGroup* p_debug_group = nullptr) final;
    void  SetProgramBindings(ProgramBindings& program_bindings, ProgramBindings::ApplyBehavior apply_behavior) override;
    void  Commit() override;
    void  WaitUntilCompleted(uint32_t timeout_ms = 0U) override;
    Data::TimeRange GetGpuTimeRange(bool) const override { return { 0U, 0U }; }
    CommandQueue& GetCommandQueue() override;

    // CommandListBase interface
    virtual void Execute(const CompletedCallback& completed_callback = {});
    virtual void Complete(); // Called from command queue thread, which is tracking GPU execution

    DebugGroupBase* GetTopOpenDebugGroup() const;
    void PushOpenDebugGroup(DebugGroup& debug_group);
    void ClearOpenDebugGroups();

    CommandQueueBase&               GetCommandQueueBase();
    const CommandQueueBase&         GetCommandQueueBase() const;
    const ProgramBindingsBase*      GetProgramBindingsPtr() const noexcept  { return GetCommandState().program_bindings_ptr; }
    Ptr<CommandListBase>            GetCommandListPtr()                     { return GetPtr<CommandListBase>(); }

    inline void RetainResource(const Ptr<ObjectBase>& resource_ptr)      { if (resource_ptr) m_command_state.retained_resources.emplace_back(resource_ptr); }
    inline void RetainResource(ObjectBase& resource)                     { m_command_state.retained_resources.emplace_back(resource.GetBasePtr()); }

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<ObjectBase, T>>>
    inline void RetainResources(const Ptrs<T>& resource_ptrs)
    {
        for(const Ptr<T>& resource_ptr : resource_ptrs)
            RetainResource(std::static_pointer_cast<ObjectBase>(resource_ptr));
    }

protected:
    virtual void ResetCommandState();
    virtual void ApplyProgramBindings(ProgramBindingsBase& program_bindings, ProgramBindings::ApplyBehavior apply_behavior);

    CommandState&       GetCommandState()               { return m_command_state; }
    const CommandState& GetCommandState() const         { return m_command_state; }

    void        SetCommandListState(State state);
    void        SetCommandListStateNoLock(State state);
    bool        IsExecutingOnAnyFrame() const           { return m_state == State::Executing; }
    bool        IsCommitted() const                     { return m_state == State::Committed; }
    bool        IsExecuting() const                     { return m_state == State::Executing; }

    inline void VerifyEncodingState() const
    {
        META_CHECK_ARG_EQUAL_DESCR(m_state, State::Encoding,
                                   "{} command list '{}' encoding is not possible in '{}' state",
                                   magic_enum::enum_name(m_type), GetName(), magic_enum::enum_name(m_state));
    }

private:
    using DebugGroupStack  = std::stack<Ptr<DebugGroupBase>>;

    void CompleteInternal();

    const Type                  m_type;
    Ptr<CommandQueueBase>       m_command_queue_ptr;
    CommandState                m_command_state;
    DebugGroupStack             m_open_debug_groups;
    CompletedCallback           m_completed_callback;
    State                       m_state = State::Pending;
    mutable TracyLockable(std::mutex, m_state_mutex)
    TracyLockable(std::mutex,   m_state_change_mutex)
    std::condition_variable_any m_state_change_condition_var;
    TRACY_GPU_SCOPE_TYPE        m_tracy_gpu_scope;
};

class CommandListSetBase
    : public CommandListSet
    , protected Data::Receiver<IObjectCallback>
    , public std::enable_shared_from_this<CommandListSetBase>
{
public:
    explicit CommandListSetBase(const Refs<CommandList>& command_list_refs, Opt<Data::Index> frame_index_opt);

    // CommandListSet overrides
    Data::Size               GetCount() const noexcept final        { return static_cast<Data::Size>(m_refs.size()); }
    const Refs<CommandList>& GetRefs() const noexcept final         { return m_refs; }
    CommandList&             operator[](Data::Index index) const final;
    const Opt<Data::Index>&  GetFrameIndex() const noexcept final   { return m_frame_index_opt; }

    // CommandListSetBase interface
    virtual void Execute(const CommandList::CompletedCallback& completed_callback);
    virtual void WaitUntilCompleted() = 0;

    bool IsExecuting() const noexcept { return m_is_executing; }
    void Complete() const;

    [[nodiscard]] Ptr<CommandListSetBase>      GetPtr()                                     { return shared_from_this(); }
    [[nodiscard]] const Refs<CommandListBase>& GetBaseRefs() const noexcept                 { return m_base_refs; }
    [[nodiscard]] const CommandListBase&       GetCommandListBase(Data::Index index) const;
    [[nodiscard]] CommandQueueBase&            GetCommandQueueBase()                        { return m_base_refs.back().get().GetCommandQueueBase(); }
    [[nodiscard]] const CommandQueueBase&      GetCommandQueueBase() const                  { return m_base_refs.back().get().GetCommandQueueBase(); }
    [[nodiscard]] const std::string&           GetCombinedName();

protected:
    // IObjectCallback interface
    void OnObjectNameChanged(Object&, const std::string&) override;

private:
    Refs<CommandList>     m_refs;
    Refs<CommandListBase> m_base_refs;
    Ptrs<CommandListBase> m_base_ptrs;
    Opt<Data::Index>      m_frame_index_opt;
    std::string           m_combined_name;

    mutable TracyLockable(std::mutex, m_command_lists_mutex)
    mutable std::atomic<bool> m_is_executing = false;
};

} // namespace Methane::Graphics
