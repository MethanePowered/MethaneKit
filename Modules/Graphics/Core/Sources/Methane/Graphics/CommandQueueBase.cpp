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

#include <cassert>

namespace Methane::Graphics
{

CommandQueueBase::CommandQueueBase(ContextBase& context)
    : m_context(context)
{
    META_FUNCTION_TASK();
}

CommandQueueBase::~CommandQueueBase()
{
    META_FUNCTION_TASK();
}

void CommandQueueBase::Execute(CommandLists& command_lists)
{
    META_FUNCTION_TASK();

    const uint32_t frame_index = GetCurrentFrameBufferIndex();
    META_LOG("Command queue \"" + GetName() + "\" is executing on frame " + std::to_string(frame_index));

    static_cast<CommandListsBase&>(command_lists).Execute(frame_index);
}

uint32_t CommandQueueBase::GetCurrentFrameBufferIndex() const
{
    return m_context.GetType() == Context::Type::Render
                                ? dynamic_cast<const RenderContextBase&>(m_context).GetFrameBufferIndex()
                                : 0u;
}

} // namespace Methane::Graphics
