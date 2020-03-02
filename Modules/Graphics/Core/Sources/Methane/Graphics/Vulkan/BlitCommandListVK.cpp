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

FILE: Methane/Graphics/Metal/BlitCommandListVK.cpp
Vulkan implementation of the blit command list interface.

******************************************************************************/

#include "BlitCommandListVK.h"
#include "CommandQueueVK.h"

#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

Ptr<BlitCommandList> BlitCommandList::Create(CommandQueue& command_queue)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<BlitCommandListVK>(static_cast<CommandQueueBase&>(command_queue));
}

BlitCommandListVK::BlitCommandListVK(CommandQueueBase& command_queue)
    : CommandListBase(command_queue, CommandList::Type::Blit)
{
    ITT_FUNCTION_TASK();
}

void BlitCommandListVK::Reset(const std::string& debug_group)
{
    ITT_FUNCTION_TASK();
}

void BlitCommandListVK::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    CommandListBase::SetName(name);
}

void BlitCommandListVK::PushDebugGroup(const std::string& name)
{
    ITT_FUNCTION_TASK();
}

void BlitCommandListVK::PopDebugGroup()
{
    ITT_FUNCTION_TASK();
}

void BlitCommandListVK::Commit()
{
    ITT_FUNCTION_TASK();
    
    assert(!IsCommitted());
    CommandListBase::Commit();
}

void BlitCommandListVK::Execute(uint32_t frame_index)
{
    ITT_FUNCTION_TASK();

    CommandListBase::Execute(frame_index);
}

CommandQueueVK& BlitCommandListVK::GetCommandQueueVK() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<class CommandQueueVK&>(GetCommandQueue());
}

} // namespace Methane::Graphics
