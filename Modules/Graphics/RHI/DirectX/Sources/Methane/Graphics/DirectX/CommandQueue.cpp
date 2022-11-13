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

FILE: Methane/Graphics/DirectX/CommandQueue.cpp
DirectX 12 implementation of the command queue interface.

******************************************************************************/

#include <Methane/Graphics/DirectX/CommandQueue.h>
#include <Methane/Graphics/DirectX/CommandListSet.h>
#include <Methane/Graphics/DirectX/Device.h>
#include <Methane/Graphics/DirectX/TransferCommandList.h>
#include <Methane/Graphics/DirectX/RenderCommandList.h>
#include <Methane/Graphics/DirectX/ParallelRenderCommandList.h>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/Windows/DirectXErrorHandling.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <nowide/convert.hpp>
#include <magic_enum.hpp>
#include <stdexcept>
#include <cassert>

namespace Methane::Graphics::Rhi
{

Ptr<ICommandQueue> Rhi::ICommandQueue::Create(const Rhi::IContext& context, CommandListType command_lists_type)
{
    META_FUNCTION_TASK();
    auto command_queue_ptr =  std::make_shared<DirectX::CommandQueue>(dynamic_cast<const Base::Context&>(context), command_lists_type);
#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
    // Base::TimestampQueryPool construction uses command queue and requires it to be fully constructed
    command_queue_ptr->InitializeTimestampQueryPool();
#endif
    return command_queue_ptr;
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::DirectX
{

static D3D12_COMMAND_LIST_TYPE GetNativeCommandListType(Rhi::CommandListType command_list_type, Rhi::IContext::Options options)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    switch(command_list_type)
    {
    case Rhi::CommandListType::Transfer:
        return static_cast<bool>(options & Rhi::IContext::Options::TransferWithDirectQueueOnWindows)
             ? D3D12_COMMAND_LIST_TYPE_DIRECT
             : D3D12_COMMAND_LIST_TYPE_COPY;

    case Rhi::CommandListType::Render:
    case Rhi::CommandListType::ParallelRender:
        return D3D12_COMMAND_LIST_TYPE_DIRECT;

    default:
        META_UNEXPECTED_ARG_RETURN(command_list_type, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }
}

static wrl::ComPtr<ID3D12CommandQueue> CreateNativeCommandQueue(const Device& device, D3D12_COMMAND_LIST_TYPE command_list_type)
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

CommandQueue::CommandQueue(const Base::Context& context, Rhi::CommandListType command_lists_type)
    : Base::CommandQueueTracking(context, command_lists_type)
    , m_dx_context(dynamic_cast<const IContextDx&>(context))
    , m_cp_command_queue(CreateNativeCommandQueue(m_dx_context.GetDirectDevice(), GetNativeCommandListType(command_lists_type, context.GetOptions())))
{
    META_FUNCTION_TASK();
#if defined(METHANE_GPU_INSTRUMENTATION_ENABLED) && METHANE_GPU_INSTRUMENTATION_ENABLED == 2
    m_tracy_context = TracyD3D12Context(GetDirectContext().GetDirectDevice().GetNativeDevice().Get(), m_cp_command_queue.Get());
#endif
}

CommandQueue::~CommandQueue()
{
    META_FUNCTION_TASK();
    ShutdownQueueExecution();
#if defined(METHANE_GPU_INSTRUMENTATION_ENABLED) && METHANE_GPU_INSTRUMENTATION_ENABLED == 2
    TracyD3D12Destroy(m_tracy_context);
#endif
}

bool CommandQueue::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (name == GetName())
        return false;

    Base::CommandQueueTracking::SetName(name);
    m_cp_command_queue->SetName(nowide::widen(name).c_str());

#if defined(METHANE_GPU_INSTRUMENTATION_ENABLED) && METHANE_GPU_INSTRUMENTATION_ENABLED == 2
    TracyD3D12ContextName(m_tracy_context, GetName().c_str(), static_cast<uint16_t>(name.length()));
#endif

    return true;
}

#if defined(METHANE_GPU_INSTRUMENTATION_ENABLED) && METHANE_GPU_INSTRUMENTATION_ENABLED == 2
void CommandQueue::CompleteExecution(const Opt<Data::Index>& frame_index)
{
    META_FUNCTION_TASK();
    Base::CommandQueueTracking::CompleteExecution(frame_index);

    TracyD3D12Collect(m_tracy_context);
    if (frame_index)
    {
        TracyD3D12NewFrame(m_tracy_context);
    }
}
#endif

ID3D12CommandQueue& CommandQueue::GetNativeCommandQueue()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_cp_command_queue);
    return *m_cp_command_queue.Get();
}

} // namespace Methane::Graphics::DirectX
