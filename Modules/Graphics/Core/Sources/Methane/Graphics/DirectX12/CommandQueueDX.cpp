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

FILE: Methane/Graphics/DirectX12/CommandQueueDX.cpp
DirectX 12 implementation of the command queue interface.

******************************************************************************/

#include "CommandQueueDX.h"
#include "CommandListDX.h"
#include "DeviceDX.h"
#include "TransferCommandListDX.h"
#include "RenderCommandListDX.h"
#include "ParallelRenderCommandListDX.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/Windows/DirectXErrorHandling.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <nowide/convert.hpp>
#include <magic_enum.hpp>
#include <stdexcept>
#include <cassert>

namespace Methane::Graphics
{

Ptr<CommandQueue> CommandQueue::Create(const Context& context, CommandList::Type command_lists_type)
{
    META_FUNCTION_TASK();
    auto command_queue_ptr =  std::make_shared<CommandQueueDX>(dynamic_cast<const ContextBase&>(context), command_lists_type);
#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
    // TimestampQueryBuffer construction uses command queue and requires it to be fully constructed
    command_queue_ptr->InitializeTimestampQueryBuffer();
#endif
    return command_queue_ptr;
}

static D3D12_COMMAND_LIST_TYPE GetNativeCommandListType(CommandList::Type command_list_type, Context::Options options)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    switch(command_list_type)
    {
    case CommandList::Type::Transfer:
        return static_cast<bool>(options & Context::Options::TransferWithDirectQueueOnWindows)
             ? D3D12_COMMAND_LIST_TYPE_DIRECT
             : D3D12_COMMAND_LIST_TYPE_COPY;

    case CommandList::Type::Render:
    case CommandList::Type::ParallelRender:
        return D3D12_COMMAND_LIST_TYPE_DIRECT;

    default:
        META_UNEXPECTED_ARG_RETURN(command_list_type, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }
}

static wrl::ComPtr<ID3D12CommandQueue> CreateNativeCommandQueue(const DeviceDX& device, D3D12_COMMAND_LIST_TYPE command_list_type)
{
    META_FUNCTION_TASK();
    const wrl::ComPtr<ID3D12Device>& cp_device = device.GetNativeDevice();
    META_CHECK_ARG_NOT_NULL(cp_device);

    D3D12_COMMAND_QUEUE_DESC queue_desc{};
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.Type = command_list_type;

    wrl::ComPtr<ID3D12CommandQueue> cp_command_queue;
    ThrowIfFailed(cp_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&cp_command_queue)), cp_device.Get());
    return cp_command_queue;
}

CommandQueueDX::CommandQueueDX(const ContextBase& context, CommandList::Type command_lists_type)
    : CommandQueueTrackingBase(context, command_lists_type)
    , m_cp_command_queue(CreateNativeCommandQueue(GetContextDX().GetDeviceDX(), GetNativeCommandListType(command_lists_type, context.GetOptions())))
{
    META_FUNCTION_TASK();
#if defined(METHANE_GPU_INSTRUMENTATION_ENABLED) && METHANE_GPU_INSTRUMENTATION_ENABLED == 2
    m_tracy_context = TracyD3D12Context(GetContextDX().GetDeviceDX().GetNativeDevice().Get(), m_cp_command_queue.Get());
#endif
}

CommandQueueDX::~CommandQueueDX()
{
    META_FUNCTION_TASK();
    ShutdownQueueExecution();
#if defined(METHANE_GPU_INSTRUMENTATION_ENABLED) && METHANE_GPU_INSTRUMENTATION_ENABLED == 2
    TracyD3D12Destroy(m_tracy_context);
#endif
}

bool CommandQueueDX::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (name == GetName())
        return false;

    CommandQueueTrackingBase::SetName(name);
    m_cp_command_queue->SetName(nowide::widen(name).c_str());

#if defined(METHANE_GPU_INSTRUMENTATION_ENABLED) && METHANE_GPU_INSTRUMENTATION_ENABLED == 2
    TracyD3D12ContextName(m_tracy_context, GetName().c_str(), static_cast<uint16_t>(name.length()));
#endif

    return true;
}

#if defined(METHANE_GPU_INSTRUMENTATION_ENABLED) && METHANE_GPU_INSTRUMENTATION_ENABLED == 2
void CommandQueueDX::CompleteExecution(const Opt<Data::Index>& frame_index)
{
    META_FUNCTION_TASK();
    CommandQueueTrackingBase::CompleteExecution(frame_index);

    TracyD3D12Collect(m_tracy_context);
    if (frame_index)
    {
        TracyD3D12NewFrame(m_tracy_context);
    }
}
#endif

const IContextDX& CommandQueueDX::GetContextDX() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const IContextDX&>(GetContextBase());
}

ID3D12CommandQueue& CommandQueueDX::GetNativeCommandQueue()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_cp_command_queue);
    return *m_cp_command_queue.Get();
}

} // namespace Methane::Graphics
