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

FILE: Methane/Graphics/DirectX12/CommandQueueDX.cpp
DirectX 12 implementation of the command queue interface.

******************************************************************************/

#include "CommandQueueDX.h"
#include "DeviceDX.h"
#include "BlitCommandListDX.h"
#include "RenderCommandListDX.h"
#include "ParallelRenderCommandListDX.h"

#include <Methane/Instrumentation.h>
#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/Windows/Primitives.h>

#include <nowide/convert.hpp>
#include <cassert>

namespace Methane::Graphics
{

Ptr<CommandQueue> CommandQueue::Create(Context& context)
{
    META_FUNCTION_TASK();
    return std::make_shared<CommandQueueDX>(dynamic_cast<ContextBase&>(context));
}

CommandQueueDX::CommandQueueDX(ContextBase& context)
    : CommandQueueBase(context)
{
    META_FUNCTION_TASK();

    const wrl::ComPtr<ID3D12Device>& cp_device = GetContextDX().GetDeviceDX().GetNativeDevice();
    assert(!!cp_device);

    D3D12_COMMAND_QUEUE_DESC queue_desc{};
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(cp_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_cp_command_queue)), cp_device.Get());
}

void CommandQueueDX::SetName(const std::string& name)
{
    META_FUNCTION_TASK();

    CommandQueueBase::SetName(name);
    m_cp_command_queue->SetName(nowide::widen(name).c_str());
}

void CommandQueueDX::Execute(const CommandLists& command_lists)
{
    META_FUNCTION_TASK();
    CommandQueueBase::Execute(command_lists);

    const CommandListsDX& dx_command_lists = static_cast<const CommandListsDX&>(command_lists);
    const CommandListsDX::NativeCommandLists& native_command_lists = dx_command_lists.GetNativeCommandLists();
    m_cp_command_queue->ExecuteCommandLists(static_cast<UINT>(native_command_lists.size()), native_command_lists.data());
}

IContextDX& CommandQueueDX::GetContextDX() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<IContextDX&>(GetContext());
}

ID3D12CommandQueue& CommandQueueDX::GetNativeCommandQueue()
{
    META_FUNCTION_TASK();
    assert(!!m_cp_command_queue);
    return *m_cp_command_queue.Get();
}

} // namespace Methane::Graphics
