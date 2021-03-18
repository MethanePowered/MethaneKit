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

FILE: Methane/Graphics/DirectX12/BlitCommandListDX.cpp
DirectX 12 implementation of the blit command list interface.

******************************************************************************/

#include "BlitCommandListDX.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/CommandQueueBase.h>
#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>

namespace Methane::Graphics
{

static D3D12_COMMAND_LIST_TYPE GetBlitCommandListNativeType(Context::Options options)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;
    return magic_enum::flags::enum_contains(options & Context::Options::BlitWithCopyQueueOnWindows)
         ? D3D12_COMMAND_LIST_TYPE_COPY
         : D3D12_COMMAND_LIST_TYPE_DIRECT;
}

Ptr<BlitCommandList> BlitCommandList::Create(CommandQueue& cmd_queue)
{
    META_FUNCTION_TASK();
    return std::make_shared<BlitCommandListDX>(static_cast<CommandQueueBase&>(cmd_queue));
}

BlitCommandListDX::BlitCommandListDX(CommandQueueBase& cmd_queue)
    : CommandListDX<CommandListBase>(GetBlitCommandListNativeType(cmd_queue.GetContext().GetOptions()), cmd_queue, Type::Blit)
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics
