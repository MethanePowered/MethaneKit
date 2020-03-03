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

FILE: Methane/Graphics/CommandQueueBase.cpp
Base implementation of the command queue interface.

******************************************************************************/

#include "CommandQueueBase.h"
#include "RenderContextBase.h"

#include <Methane/Instrumentation.h>

#ifdef COMMAND_EXECUTION_LOGGING
#include <Methane/Platform/Utils.h>
#endif

#include <cassert>

namespace Methane::Graphics
{

CommandQueueBase::CommandQueueBase(ContextBase& context)
    : m_context(context)
{
    ITT_FUNCTION_TASK();
}

CommandQueueBase::~CommandQueueBase()
{
    ITT_FUNCTION_TASK();
}

void CommandQueueBase::Execute(const Refs<CommandList>& command_lists)
{
    ITT_FUNCTION_TASK();

    const uint32_t frame_index = GetCurrentFrameBufferIndex();

#ifdef COMMAND_EXECUTION_LOGGING
        Platform::PrintToDebugOutput("CommandQueue \"" + GetName() + "\" is executing on frame " + std::to_string(frame_index));
#endif

    for (const auto& command_list_ref : command_lists)
    {
        if (std::addressof(command_list_ref.get().GetCommandQueue()) != std::addressof(*this))
        {
            throw std::runtime_error("Can not execute command list created in different command queue.");
        }

        CommandListBase& command_list = dynamic_cast<CommandListBase&>(command_list_ref.get());
        command_list.Execute(frame_index);
    }
}

uint32_t CommandQueueBase::GetCurrentFrameBufferIndex() const
{
    return m_context.GetType() == Context::Type::Render
                                ? dynamic_cast<const RenderContextBase&>(m_context).GetFrameBufferIndex()
                                : 0u;
}

} // namespace Methane::Graphics
