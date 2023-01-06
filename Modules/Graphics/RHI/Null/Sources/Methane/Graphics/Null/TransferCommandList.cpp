/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Null/TransferCommandList.cpp
Null implementation of the transfer command list interface.

******************************************************************************/

#include <Methane/Graphics/Null/TransferCommandList.h>
#include <Methane/Graphics/Null/CommandQueue.h>

namespace Methane::Graphics::Rhi
{

Ptr<ITransferCommandList> Rhi::ITransferCommandList::Create(ICommandQueue& command_queue)
{
    return std::make_shared<Null::TransferCommandList>(static_cast<Null::CommandQueue&>(command_queue));
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Null
{

TransferCommandList::TransferCommandList(CommandQueue& command_queue)
    : CommandList(command_queue, Rhi::CommandListType::Transfer)
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics::Null
