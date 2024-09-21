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
#include <Methane/Graphics/DirectX/Device.h>
#include <Methane/Graphics/DirectX/Fence.h>
#include <Methane/Graphics/DirectX/TransferCommandList.h>
#include <Methane/Graphics/DirectX/ComputeCommandList.h>
#include <Methane/Graphics/DirectX/RenderCommandList.h>
#include <Methane/Graphics/DirectX/ParallelRenderCommandList.h>
#include <Methane/Graphics/DirectX/QueryPool.h>
#include <Methane/Graphics/DirectX/ICommandList.h>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/DirectX/ErrorHandling.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <nowide/convert.hpp>
#include <magic_enum.hpp>
#include <stdexcept>
#include <cassert>

namespace Methane::Graphics::DirectX
{

static bool CheckCommandQueueSupportsTimestampQueries(CommandQueue& command_queue)
{
    META_FUNCTION_TASK();
    if (command_queue.GetNativeCommandQueue().GetDesc().Type != D3D12_COMMAND_LIST_TYPE_COPY)
        return true;

    if (D3D12_FEATURE_DATA_D3D12_OPTIONS3 feature_data{};
        SUCCEEDED(command_queue.GetDirectContext().GetDirectDevice().GetNativeDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &feature_data, sizeof(feature_data))))
        return static_cast<bool>(feature_data.CopyQueueTimestampQueriesSupported);

    return true;
}

static D3D12_COMMAND_LIST_TYPE GetNativeCommandListType(Rhi::CommandListType command_list_type, Rhi::ContextOptionMask options)
{
    META_FUNCTION_TASK();
    switch(command_list_type)
    {
    case Rhi::CommandListType::Transfer:
        return options.HasBit(Rhi::ContextOption::TransferWithD3D12DirectQueue)
             ? D3D12_COMMAND_LIST_TYPE_DIRECT
             : D3D12_COMMAND_LIST_TYPE_COPY;

    case Rhi::CommandListType::Render:
    case Rhi::CommandListType::ParallelRender:
        return D3D12_COMMAND_LIST_TYPE_DIRECT;

    case Rhi::CommandListType::Compute:
        return D3D12_COMMAND_LIST_TYPE_COMPUTE;

    default:
        META_UNEXPECTED_RETURN(command_list_type, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }
}

static wrl::ComPtr<ID3D12CommandQueue> CreateNativeCommandQueue(const Device& device, D3D12_COMMAND_LIST_TYPE command_list_type)
{
    META_FUNCTION_TASK();
    const wrl::ComPtr<ID3D12Device>& device_cptr = device.GetNativeDevice();
    META_CHECK_NOT_NULL(device_cptr);

    D3D12_COMMAND_QUEUE_DESC queue_desc{};
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.Type = command_list_type;

    wrl::ComPtr<ID3D12CommandQueue> command_queue_cptr;
    ThrowIfFailed(device_cptr->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue_cptr)), device_cptr.Get());
    return command_queue_cptr;
}

CommandQueue::CommandQueue(const Base::Context& context, Rhi::CommandListType command_lists_type)
    : Base::CommandQueueTracking(context, command_lists_type)
    , m_dx_context(dynamic_cast<const IContext&>(context))
    , m_command_queue_cptr(CreateNativeCommandQueue(m_dx_context.GetDirectDevice(), GetNativeCommandListType(command_lists_type, context.GetOptions())))
{
    META_FUNCTION_TASK();
#if defined(METHANE_GPU_INSTRUMENTATION_ENABLED) && METHANE_GPU_INSTRUMENTATION_ENABLED == 2
    m_tracy_context = TracyD3D12Context(GetDirectContext().GetDirectDevice().GetNativeDevice().Get(), m_command_queue_cptr.Get());
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

Ptr<Rhi::IFence> CommandQueue::CreateFence()
{
    META_FUNCTION_TASK();
    return std::make_shared<Fence>(*this);
}

Ptr<Rhi::ITransferCommandList> CommandQueue::CreateTransferCommandList()
{
    META_FUNCTION_TASK();
    return std::make_shared<TransferCommandList>(*this);
}

Ptr<Rhi::IComputeCommandList> CommandQueue::CreateComputeCommandList()
{
    META_FUNCTION_TASK();
    return std::make_shared<ComputeCommandList>(*this);
}

Ptr<Rhi::IRenderCommandList> CommandQueue::CreateRenderCommandList(Rhi::IRenderPass& render_pass)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderCommandList>(*this, dynamic_cast<RenderPass&>(render_pass));
}

Ptr<Rhi::IParallelRenderCommandList> CommandQueue::CreateParallelRenderCommandList(Rhi::IRenderPass& render_pass)
{
    META_FUNCTION_TASK();
    return std::make_shared<ParallelRenderCommandList>(*this, dynamic_cast<RenderPass&>(render_pass));
}

Ptr<Rhi::ITimestampQueryPool> CommandQueue::CreateTimestampQueryPool(uint32_t max_timestamps_per_frame)
{
    META_FUNCTION_TASK();
    return CheckCommandQueueSupportsTimestampQueries(*this)
         ? std::make_shared<TimestampQueryPool>(*this, max_timestamps_per_frame)
         : nullptr;
}

bool CommandQueue::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (name == GetName())
        return false;

    Base::CommandQueueTracking::SetName(name);
    m_command_queue_cptr->SetName(nowide::widen(name).c_str());

#if defined(METHANE_GPU_INSTRUMENTATION_ENABLED) && METHANE_GPU_INSTRUMENTATION_ENABLED == 2
    TracyD3D12ContextName(m_tracy_context, GetName().data(), static_cast<uint16_t>(name.length()));
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
    META_CHECK_NOT_NULL(m_command_queue_cptr);
    return *m_command_queue_cptr.Get();
}

} // namespace Methane::Graphics::DirectX
