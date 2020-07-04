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

FILE: Methane/Graphics/Metal/BlitCommandListMT.mm
Metal implementation of the blit command list interface.

******************************************************************************/

#include "BlitCommandListMT.hh"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<BlitCommandList> BlitCommandList::Create(CommandQueue& command_queue)
{
    META_FUNCTION_TASK();
    return std::make_shared<BlitCommandListMT>(static_cast<CommandQueueBase&>(command_queue));
}

BlitCommandListMT::BlitCommandListMT(CommandQueueBase& command_queue)
    : CommandListMT<id<MTLBlitCommandEncoder>, CommandListBase>(true, command_queue, CommandList::Type::Blit)
{
    META_FUNCTION_TASK();
}

void BlitCommandListMT::Reset(CommandList::DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();

    CommandListBase::ResetCommandState();

    if (IsCommandEncoderInitialized())
    {
        CommandListBase::Reset(p_debug_group);
        return;
    }

    id<MTLCommandBuffer>& mtl_cmd_buffer = InitializeCommandBuffer();
    InitializeCommandEncoder([mtl_cmd_buffer blitCommandEncoder]);
    CommandListBase::Reset(p_debug_group);
}

} // namespace Methane::Graphics
