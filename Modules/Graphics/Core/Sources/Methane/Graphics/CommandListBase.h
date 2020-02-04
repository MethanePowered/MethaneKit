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

    CommandListBase(CommandQueueBase& command_queue, Type type);

    // CommandList interface
    Type GetType() const override { return m_type; }
    void Reset(const std::string& debug_group = "") override;
    void Commit(bool present_drawable) override;
    CommandQueue& GetCommandQueue() override;

    // CommandListBase interface

    virtual void SetResourceBarriers(const ResourceBase::Barriers& resource_barriers) = 0;
    virtual void Execute(uint32_t frame_index);
    virtual void Complete(uint32_t frame_index);

    void SetResourceTransitionBarriers(const Refs<Resource>& resources, ResourceBase::State state_before, ResourceBase::State state_after);
    Ptr<CommandListBase> GetPtr()                     { return shared_from_this(); }
    void SetDebugGroupOpened(bool debug_group_opened) { m_debug_group_opened = debug_group_opened; }

protected:
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

    const Type           m_type;
    Ptr<CommandListBase> m_sp_self;
    Ptr<CommandQueue>    m_sp_command_queue;
    bool                 m_debug_group_opened = false;
    uint32_t             m_committed_frame_index = 0;
    State                m_state                 = State::Pending;
    mutable std::mutex   m_state_mutex;
};

} // namespace Methane::Graphics
