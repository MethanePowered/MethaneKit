/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/DirectX12/SyncCommandListDX.cpp
DirectX 12 implementation of the synchronization command list interface.

******************************************************************************/

#include "SyncCommandListDX.h"

#include <Methane/Graphics/CommandQueueBase.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<SyncCommandList> SyncCommandList::Create(CommandQueue& cmd_queue)
{
    META_FUNCTION_TASK();
    return std::make_shared<SyncCommandListDX>(static_cast<CommandQueueBase&>(cmd_queue));
}

SyncCommandListDX::SyncCommandListDX(CommandQueueBase& cmd_queue)
    : CommandListDX<CommandListBase>(D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_queue, Type::Sync)
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics
