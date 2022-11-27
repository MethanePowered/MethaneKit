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

FILE: Methane/Graphics/DirectX/TransferCommandList.cpp
DirectX 12 implementation of the transfer command list interface.

******************************************************************************/

#include <Methane/Graphics/DirectX/TransferCommandList.h>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/Base/CommandQueue.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

Ptr<ITransferCommandList> ITransferCommandList::Create(ICommandQueue& cmd_queue)
{
    META_FUNCTION_TASK();
    return std::make_shared<DirectX::TransferCommandList>(static_cast<Base::CommandQueue&>(cmd_queue));
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::DirectX
{

static D3D12_COMMAND_LIST_TYPE GetTransferCommandListNativeType(Rhi::ContextOptionMask options)
{
    META_FUNCTION_TASK();
    return options.HasBit(Rhi::ContextOption::TransferWithD3D12DirectQueue)
         ? D3D12_COMMAND_LIST_TYPE_DIRECT
         : D3D12_COMMAND_LIST_TYPE_COPY;
}

TransferCommandList::TransferCommandList(Base::CommandQueue& cmd_queue)
    : CommandList<Base::CommandList>(GetTransferCommandListNativeType(cmd_queue.GetContext().GetOptions()), cmd_queue, Type::Transfer)
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics::DirectX
