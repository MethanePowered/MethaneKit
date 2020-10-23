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

FILE: Methane/Graphics/CommandQueueBase.h
Base implementation of the command queue interface.

******************************************************************************/

#pragma once

#include "ObjectBase.h"
#include "CommandListBase.h"

#include <Methane/Graphics/CommandQueue.h>
#include <Methane/TracyGpu.hpp>

#include <list>
#include <set>
#include <mutex>

namespace Methane::Graphics
{

class RenderContextBase;

class CommandQueueBase
    : public ObjectBase
    , public CommandQueue
{
    friend class CommandListBase;

public:
    CommandQueueBase(ContextBase& context, CommandList::Type command_lists_type);
    ~CommandQueueBase() override;

    // CommandQueue interface
    void Execute(CommandListSet& command_lists, const CommandList::CompletedCallback& completed_callback = {}) override;
    CommandList::Type GetCommandListsType() const noexcept override { return m_command_lists_type; }

    Ptr<CommandQueueBase> GetCommandQueuePtr()          { return std::static_pointer_cast<CommandQueueBase>(GetBasePtr()); }
    ContextBase&          GetContext() const noexcept   { return m_context; }
    Tracy::GpuContext&    GetTracyContext() noexcept;

protected:
    void InitializeTracyGpuContext(const Tracy::GpuContext::Settings& tracy_settings);
    uint32_t GetCurrentFrameBufferIndex() const;

private:
    ContextBase&                 m_context;
    const CommandList::Type      m_command_lists_type;
    UniquePtr<Tracy::GpuContext> m_tracy_gpu_context_ptr;
};

} // namespace Methane::Graphics
