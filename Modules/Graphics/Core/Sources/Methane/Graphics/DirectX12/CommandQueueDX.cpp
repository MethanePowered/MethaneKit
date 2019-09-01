/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/DirectX12/CommandQueueDX.cpp
DirectX 12 implementation of the command queue interface.

******************************************************************************/

#include "CommandQueueDX.h"
#include "ContextDX.h"
#include "DeviceDX.h"
#include "RenderCommandListDX.h"

#include <Methane/Instrumentation.h>
#include <Methane/Graphics/Windows/Helpers.h>

#include <nowide/convert.hpp>

#include <cassert>

namespace Methane
{
namespace Graphics
{

CommandQueue::Ptr CommandQueue::Create(Context& context)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<CommandQueueDX>(static_cast<ContextBase&>(context));
}

CommandQueueDX::CommandQueueDX(ContextBase& context)
    : CommandQueueBase(context, false)
{
    ITT_FUNCTION_TASK();

    const wrl::ComPtr<ID3D12Device>& cp_device = GetContextDX().GetDeviceDX().GetNativeDevice();
    assert(!!cp_device);

    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(cp_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_cp_command_queue)));
}

void CommandQueueDX::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    CommandQueueBase::SetName(name);
    m_cp_command_queue->SetName(nowide::widen(name).c_str());
}

void CommandQueueDX::Execute(const CommandList::Refs& command_lists)
{
    ITT_FUNCTION_TASK();
    assert(!command_lists.empty());
    if (command_lists.empty())
        return;

    CommandQueueBase::Execute(command_lists);

    D3D12CommandLists dx_command_lists = GetNativeCommandLists(command_lists);
    m_cp_command_queue->ExecuteCommandLists(static_cast<UINT>(dx_command_lists.size()), dx_command_lists.data());
}

CommandQueueDX::D3D12CommandLists CommandQueueDX::GetNativeCommandLists(const CommandList::Refs& command_list_refs)
{
    ITT_FUNCTION_TASK();
    D3D12CommandLists dx_command_lists;
    dx_command_lists.reserve(command_list_refs.size());
    for (const CommandList::Ref& command_list_ref : command_list_refs)
    {
        RenderCommandListDX& dx_command_list = dynamic_cast<RenderCommandListDX&>(command_list_ref.get());
        dx_command_lists.push_back(dx_command_list.GetNativeCommandList().Get());
    }
    return dx_command_lists;
}

ContextDX& CommandQueueDX::GetContextDX()
{
    ITT_FUNCTION_TASK();
    return static_cast<class ContextDX&>(m_context);
}

} // namespace Graphics
} // namespace Methane
