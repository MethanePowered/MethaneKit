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

FILE: Methane/Graphics/Null/CommandQueue.cpp
Null implementation of the command queue interface.

******************************************************************************/

#include <Methane/Graphics/Null/CommandQueue.h>
#include <Methane/Graphics/Null/Fence.h>
#include <Methane/Graphics/Null/TransferCommandList.h>
#include <Methane/Graphics/Null/RenderCommandList.h>
#include <Methane/Graphics/Null/ParallelRenderCommandList.h>
#include <Methane/Graphics/Base/Context.h>

namespace Methane::Graphics::Null
{

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

Ptr<Rhi::ITimestampQueryPool> CommandQueue::CreateTimestampQueryPool(uint32_t)
{
    META_FUNCTION_TASK();
    return nullptr;
}

} // namespace Methane::Graphics::Null
