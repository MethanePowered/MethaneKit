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

FILE: Methane/Graphics/Metal/CommandQueue.mm
Metal implementation of the command queue interface.

******************************************************************************/

#include <Methane/Graphics/Metal/CommandQueue.hh>
#include <Methane/Graphics/Metal/Device.hh>
#include <Methane/Graphics/Metal/Fence.hh>
#include <Methane/Graphics/Metal/TransferCommandList.hh>
#include <Methane/Graphics/Metal/ComputeCommandList.hh>
#include <Methane/Graphics/Metal/RenderCommandList.hh>
#include <Methane/Graphics/Metal/ParallelRenderCommandList.hh>
#include <Methane/Graphics/Metal/RenderContext.hh>

#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <QuartzCore/CABase.h>

namespace Methane::Graphics::Metal
{

CommandQueue::CommandQueue(const Base::Context& context, Rhi::CommandListType command_lists_type)
    : Base::CommandQueueTracking(context, command_lists_type)
    , m_mtl_command_queue([GetMetalContext().GetMetalDevice().GetNativeDevice() newCommandQueue])
{
    META_FUNCTION_TASK();
    InitializeTracyGpuContext(
        Tracy::GpuContext::Settings(
            Tracy::GpuContext::Type::Metal,
            0U,
            Data::ConvertTimeSecondsToNanoseconds(CACurrentMediaTime())
        )
    );
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
    return std::make_shared<RenderCommandList>(*this, dynamic_cast<Base::RenderPass&>(render_pass));
}

Ptr<Rhi::IParallelRenderCommandList> CommandQueue::CreateParallelRenderCommandList(Rhi::IRenderPass& render_pass)
{
    META_FUNCTION_TASK();
    return std::make_shared<ParallelRenderCommandList>(*this, dynamic_cast<Base::RenderPass&>(render_pass));
}

Ptr<Rhi::ITimestampQueryPool> CommandQueue::CreateTimestampQueryPool(uint32_t)
{
    META_FUNCTION_TASK();
    return nullptr;
}

bool CommandQueue::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Base::CommandQueue::SetName(name))
        return false;

    META_CHECK_NOT_NULL(m_mtl_command_queue);
    m_mtl_command_queue.label = MacOS::ConvertToNsString(name);
    return true;
}

const IContext& CommandQueue::GetMetalContext() const noexcept
{
    META_FUNCTION_TASK();
    return dynamic_cast<const IContext&>(GetBaseContext());
}

const RenderContext& CommandQueue::GetMetalRenderContext() const
{
    META_FUNCTION_TASK();
    const Base::Context& context = GetBaseContext();
    META_CHECK_EQUAL_DESCR(context.GetType(), Rhi::ContextType::Render, "incompatible context type");
    return static_cast<const RenderContext&>(context);
}

} // namespace Methane::Graphics::Metal
