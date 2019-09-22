/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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
    using Ptr = std::shared_ptr<CommandListBase>;
    using WeakPtr = std::weak_ptr<CommandListBase>;

    enum State
    {
        Pending,
        Committed,
        Executing,
    };

    CommandListBase(CommandQueueBase& command_queue);

    // CommandList interface
    void SetResourceBindings(const Program::ResourceBindings& resource_bindings) override;
    void Commit(bool present_drawable) override;
    CommandQueue& GetCommandQueue() override;

    // CommandListBase interface
    virtual void SetResourceBarriers(const ResourceBase::Barriers& resource_barriers) = 0;
    virtual void Execute(uint32_t frame_index);
    virtual void Complete(uint32_t frame_index);

    void SetResourceTransitionBarriers(const Resource::Refs& resources, ResourceBase::State state_before, ResourceBase::State state_after);
    Ptr  GetPtr() { return shared_from_this(); }

protected:
    CommandQueueBase&       GetCommandQueueBase();
    const CommandQueueBase& GetCommandQueueBase() const;

    bool     IsExecutingOnAnyFrame() const;
    bool     IsCommitted(uint32_t frame_index) const;
    bool     IsCommitted() const                        { return IsCommitted(GetCurrentFrameIndex()); }
    bool     IsExecuting(uint32_t frame_index) const;
    bool     IsExecuting() const                        { return IsExecuting(GetCurrentFrameIndex()); }
    uint32_t GetCurrentFrameIndex() const;

    CommandQueue::Ptr m_sp_command_queue;
    bool              m_debug_group_opened = false;

private:
    static std::string GetStateName(State state);

    using ExecutingOnFrame = std::map<uint32_t, bool>;

    Ptr                 m_sp_self;
    uint32_t            m_committed_frame_index = 0;
    State               m_state                 = State::Pending;
    mutable std::mutex  m_state_mutex;
};

} // namespace Methane::Graphics
