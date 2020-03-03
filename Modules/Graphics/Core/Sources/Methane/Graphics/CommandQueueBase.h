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

FILE: Methane/Graphics/CommandQueueBase.h
Base implementation of the command queue interface.

******************************************************************************/

#pragma once

#include "ObjectBase.h"
#include "CommandListBase.h"

#include <Methane/Graphics/CommandQueue.h>

#include <list>
#include <set>
#include <mutex>

//#define COMMAND_EXECUTION_LOGGING

namespace Methane::Graphics
{

class RenderContextBase;

class CommandQueueBase
    : public ObjectBase
    , public CommandQueue
    , public std::enable_shared_from_this<CommandQueueBase>
{
    friend class CommandListBase;

public:
    CommandQueueBase(ContextBase& context);
    ~CommandQueueBase() override;

    // CommandQueue interface
    void Execute(const Refs<CommandList>& command_lists) override;

    Ptr<CommandQueueBase> GetPtr()           { return shared_from_this(); }
    ContextBase&          GetContext()       { return m_context; }
    const ContextBase&    GetContext() const { return m_context; }

protected:
    uint32_t GetCurrentFrameBufferIndex() const;

private:
    ContextBase&        m_context;
};

} // namespace Methane::Graphics
