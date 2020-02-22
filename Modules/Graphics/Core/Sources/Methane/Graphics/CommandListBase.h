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

#include <map>
#include <mutex>

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

    struct CommandState
    {
        // NOTE:
        // Command state uses raw pointers instead of smart pointers for performance reasons:
        //   - shared pointers can not be used here, because they keep resources from deletion on context release
        //   - weak pointer should not be used too because 'lock' operation has significant performance overhead
        //   - even if raw pointer becomes obsolete it won't be a problem because it is used only for address comparison with another raw pointer
        ProgramBindingsBase* p_program_bindings = nullptr;

        static UniquePtr<CommandState> Create(Type command_list_type);

    protected:
        CommandState() = default;
    };

    CommandListBase(CommandQueueBase& command_queue, Type type);

    // CommandList interface
    Type GetType() const override { return m_type; }
    void Reset(const std::string& debug_group = "") override;
    void SetProgramBindings(ProgramBindings& program_bindings, ProgramBindings::ApplyBehavior::Mask apply_behavior) override;
    void Commit() override;
    CommandQueue& GetCommandQueue() override;

    // CommandListBase interface
    virtual void SetResourceBarriers(const ResourceBase::Barriers& resource_barriers) = 0;
    virtual void Execute(uint32_t frame_index);
    virtual void Complete(uint32_t frame_index);

    void SetResourceTransitionBarriers(const Refs<Resource>& resources, ResourceBase::State state_before, ResourceBase::State state_after);
    void SetOpenDebugGroup(const std::string& debug_group)  { m_open_debug_group = debug_group; }
    const ProgramBindingsBase* GetProgramBindings() const   { return GetCommandState().p_program_bindings; }
    Ptr<CommandListBase>       GetPtr()                     { return shared_from_this(); }

protected:
    void ResetCommandState();
    CommandState&       GetCommandState();
    const CommandState& GetCommandState() const;

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
    
    using ExecutingOnFrame = std::map<uint32_t, bool>;

    const Type              m_type;
    Ptr<CommandQueue>       m_sp_command_queue;
    UniquePtr<CommandState> m_sp_command_state;
    std::string             m_open_debug_group;
    uint32_t                m_committed_frame_index = 0;
    State                   m_state                 = State::Pending;
};

} // namespace Methane::Graphics
