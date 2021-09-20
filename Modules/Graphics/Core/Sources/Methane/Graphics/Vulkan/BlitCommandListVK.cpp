/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/BlitCommandListVK.cpp
Vulkan implementation of the blit command list interface.

******************************************************************************/

#include "BlitCommandListVK.h"
#include "CommandQueueVK.h"

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

Ptr<BlitCommandList> BlitCommandList::Create(CommandQueue& command_queue)
{
    META_FUNCTION_TASK();
#if 0
    return std::make_shared<BlitCommandListVK>(static_cast<CommandQueueVK&>(command_queue));
#else
    META_UNUSED(command_queue);
    META_FUNCTION_NOT_IMPLEMENTED_DESCR("BlitCommandList has no Vulkan API implementation yet");
#endif
}

BlitCommandListVK::BlitCommandListVK(CommandQueueVK& command_queue)
    : CommandListVK<CommandListBase>(command_queue, CommandList::Type::Blit)
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics
