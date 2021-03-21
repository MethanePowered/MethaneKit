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

FILE: Methane/Graphics/Metal/SyncCommandListVK.cpp
Vulkan implementation of the blit command list interface.

******************************************************************************/

#include "SyncCommandListVK.h"
#include "CommandQueueVK.h"

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

Ptr<SyncCommandList> SyncCommandList::Create(CommandQueue& command_queue)
{
    META_FUNCTION_TASK();
    return std::make_shared<SyncCommandListVK>(static_cast<CommandQueueBase&>(command_queue));
}

SyncCommandListVK::SyncCommandListVK(CommandQueueBase& command_queue)
    : CommandListBase(command_queue, CommandList::Type::Sync)
{
    META_FUNCTION_TASK();
}

void SyncCommandListVK::Reset(DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    CommandListBase::Reset(p_debug_group);
}

void SyncCommandListVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    CommandListBase::SetName(name);
}

void SyncCommandListVK::PushDebugGroup(DebugGroup& debug_group)
{
    META_FUNCTION_TASK();
    CommandListBase::PushDebugGroup(debug_group);
}

void SyncCommandListVK::PopDebugGroup()
{
    META_FUNCTION_TASK();
    CommandListBase::PopDebugGroup();
}

void SyncCommandListVK::Commit()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_FALSE(IsCommitted());
    CommandListBase::Commit();
}

void SyncCommandListVK::SetResourceBarriers(const Resource::Barriers&)
{
    META_FUNCTION_NOT_IMPLEMENTED();
}

void SyncCommandListVK::Execute(uint32_t frame_index, const CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    CommandListBase::Execute(frame_index, completed_callback);
}

CommandQueueVK& SyncCommandListVK::GetCommandQueueVK() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<class CommandQueueVK&>(GetCommandQueue());
}

} // namespace Methane::Graphics
