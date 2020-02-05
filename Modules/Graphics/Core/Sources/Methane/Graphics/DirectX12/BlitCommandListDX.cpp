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

FILE: Methane/Graphics/DirectX12/BlitCommandListDX.cpp
DirectX 12 implementation of the blit command list interface.

******************************************************************************/

#include "BlitCommandListDX.h"

#include <Methane/Graphics/CommandQueueBase.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<BlitCommandList> BlitCommandList::Create(CommandQueue& cmd_queue)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<BlitCommandListDX>(static_cast<CommandQueueBase&>(cmd_queue));
}

BlitCommandListDX::BlitCommandListDX(CommandQueueBase& cmd_buffer)
    : CommandListDX<CommandListBase>(cmd_buffer, Type::Blit)
{
    ITT_FUNCTION_TASK();
}

void BlitCommandListDX::Reset(const std::string& debug_group)
{
    ITT_FUNCTION_TASK();
    CommandListDX<CommandListBase>::Reset(debug_group);
}

} // namespace Methane::Graphics
