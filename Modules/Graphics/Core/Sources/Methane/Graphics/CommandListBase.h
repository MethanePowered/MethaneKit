/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
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
#include "ResourceBase.h"

#include <Methane/Graphics/Program.h>
#include <Methane/Graphics/CommandList.h>
#include <Methane/Graphics/CommandQueue.h>
#include <Methane/Memory.hpp>

#include <stack>
#include <set>

namespace Methane::Graphics
{

class CommandQueueBase;
class ProgramBindingsBase;

class CommandListBase
    : public ObjectBase
    , public virtual CommandList
    , public std::enable_shared_from_this<CommandListBase>
{
    friend class CommandQueueBase;

public:
    enum State
    {
        Pending,
        Committed,
        Executing,
    };

    struct CommandState final
    {
        // NOTE:
        // Command state uses raw pointers instead of smart pointers for performance reasons:
        //   - shared pointers can not be used here, because they keep resources from deletion on context release
        //   - weak pointer should not be used too because 'lock' operation has significant performance overhead
        //   - even if raw pointer becomes obsolete it won't be a problem because it is used only for address comparison with another raw pointer
        ProgramBindingsBase* p_program_bindings = nullptr;
    };

    CommandListBase(CommandQueueBase& command_queue, Type type);

    // CommandList interface
    Type GetType() const override { return m_type; }
    void PushDebugGroup(const std::string& name) override;
    void PopDebugGroup() override;
    void Reset(const std::string& debug_group = "") override;
    void SetProgramBindings(ProgramBindings& program_bindings, ProgramBindings::ApplyBehavior::Mask apply_behavior) override;
    void Commit() override;
    CommandQueue& GetCommandQueue() override;

    // CommandListBase interface
    virtual void SetResourceBarriers(const ResourceBase::Barriers& resource_barriers) = 0;
    virtual void Execute(uint32_t frame_index);
    virtual void Complete(uint32_t frame_index);

    const std::string& GetTopOpenDebugGroup() const;
    const std::string& PushOpenDebugGroup(const std::string& name);
    void ClearOpenDebugGroups();

    const ProgramBindingsBase* GetProgramBindings() const   { return GetCommandState().p_program_bindings; }
    Ptr<CommandListBase>       GetPtr()                     { return shared_from_this(); }

protected:
    virtual void ResetCommandState();

    CommandState&       GetCommandState()           { return m_command_state; }
    const CommandState& GetCommandState() const     { return m_command_state; }

    CommandQueueBase&       GetCommandQueueBase();
    const CommandQueueBase& GetCommandQueueBase() const;

    bool     IsExecutingOnAnyFrame() const;
    bool     IsCommitted(uint32_t frame_index) const;
    bool     IsCommitted() const                        { return IsCommitted(GetCurrentFrameIndex()); }
    bool     IsExecuting(uint32_t frame_index) const;
    bool     IsExecuting() const                        { return IsExecuting(GetCurrentFrameIndex()); }
    uint32_t GetCurrentFrameIndex() const;

private:
    static std::string GetStateName(State state);

    using GroupNamesPool   = std::set<std::string>;
    using GroupNameRef     = Ref<const std::string>;
    using GroupNamesStack  = std::stack<GroupNameRef>;

    const Type              m_type;
    Ptr<CommandQueue>       m_sp_command_queue;
    CommandState            m_command_state;
    GroupNamesPool          m_debug_group_names;
    GroupNamesStack         m_open_debug_groups;
    uint32_t                m_committed_frame_index = 0;
    State                   m_state                 = State::Pending;
};

class CommandListsBase : public CommandLists
{
public:
    CommandListsBase(Refs<CommandList> command_list_refs);

    // CommandLists interface
    Data::Size               GetCount() const noexcept override { return static_cast<Data::Size>(m_refs.size()); }
    const Refs<CommandList>& GetRefs() const noexcept override  { return m_refs; }
    CommandList&             operator[](Data::Index index) const override;

    const Refs<CommandListBase>& GetBaseRefs() const noexcept { return m_base_refs; }
    const CommandListBase& GetCommandListBase(Data::Index index) const;

private:
    Refs<CommandList>     m_refs;
    Refs<CommandListBase> m_base_refs;
    Ptrs<CommandListBase> m_base_ptrs;
};

} // namespace Methane::Graphics
