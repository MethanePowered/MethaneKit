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

FILE: Methane/Graphics/CommandQueueBase.cpp
Base implementation of the command queue interface.

******************************************************************************/

#include "CommandQueueBase.h"
#include "RenderContextBase.h"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

CommandQueueBase::CommandQueueBase(const ContextBase& context, CommandList::Type command_lists_type)
    : m_context(context)
    , m_command_lists_type(command_lists_type)
{
    META_FUNCTION_TASK();
}

CommandQueueBase::~CommandQueueBase()
{
    META_FUNCTION_TASK();
}

void CommandQueueBase::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    ObjectBase::SetName(name);
    if (m_tracy_gpu_context_ptr)
    {
        m_tracy_gpu_context_ptr->SetName(name);
    }
}

const Context& CommandQueueBase::GetContext() const noexcept
{
    META_FUNCTION_TASK();
    return dynamic_cast<const Context&>(m_context);
}

void CommandQueueBase::Execute(CommandListSet& command_lists, const CommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    const uint32_t frame_index = GetCurrentFrameBufferIndex();
    META_LOG("Command queue '{}' is executing on frame {}", GetName(), frame_index);

    static_cast<CommandListSetBase&>(command_lists).Execute(frame_index, completed_callback);
}

Tracy::GpuContext& CommandQueueBase::GetTracyContext()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_tracy_gpu_context_ptr);
    return *m_tracy_gpu_context_ptr;
}

void CommandQueueBase::InitializeTracyGpuContext(const Tracy::GpuContext::Settings& tracy_settings)
{
    META_FUNCTION_TASK();
    m_tracy_gpu_context_ptr = std::make_unique<Tracy::GpuContext>(tracy_settings);
}

uint32_t CommandQueueBase::GetCurrentFrameBufferIndex() const
{
    META_FUNCTION_TASK();
    return m_context.GetType() == Context::Type::Render
         ? dynamic_cast<const RenderContextBase&>(m_context).GetFrameBufferIndex()
         : 0U;
}

} // namespace Methane::Graphics
