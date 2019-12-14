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

FILE: Methane/Graphics/DirectX12/ParallelRenderCommandListDX.cpp
DirectX 12 implementation of the parallel render command list interface.

******************************************************************************/

#include "ParallelRenderCommandListDX.h"
#include "RenderStateDX.h"
#include "RenderPassDX.h"
#include "CommandQueueDX.h"
#include "ContextDX.h"
#include "DeviceDX.h"

#include <Methane/Data/Instrumentation.h>

#include <d3dx12.h>
#include <cassert>

namespace Methane::Graphics
{

ParallelRenderCommandList::Ptr ParallelRenderCommandList::Create(CommandQueue& cmd_queue, RenderPass& render_pass)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ParallelRenderCommandListDX>(static_cast<CommandQueueBase&>(cmd_queue), static_cast<RenderPassBase&>(render_pass));
}

ParallelRenderCommandListDX::ParallelRenderCommandListDX(CommandQueueBase& cmd_buffer, RenderPassBase& render_pass)
    : ParallelRenderCommandListBase(cmd_buffer, render_pass)
{
    ITT_FUNCTION_TASK();

    const wrl::ComPtr<ID3D12Device>& cp_device = GetCommandQueueDX().GetContextDX().GetDeviceDX().GetNativeDevice();
    assert(!!cp_device);
}

void ParallelRenderCommandListDX::Reset(RenderState& render_state)
{
    ITT_FUNCTION_TASK();

    ParallelRenderCommandListBase::Reset(render_state);

    // Reset pre-command list
}

void ParallelRenderCommandListDX::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    ParallelRenderCommandListBase::SetName(name);
}

void ParallelRenderCommandListDX::Commit(bool present_drawable)
{
    ITT_FUNCTION_TASK();

    ParallelRenderCommandListBase::Commit(present_drawable);

    m_is_committed = true;
}

CommandQueueDX& ParallelRenderCommandListDX::GetCommandQueueDX()
{
    ITT_FUNCTION_TASK();
    return static_cast<CommandQueueDX&>(GetCommandQueueBase());
}

RenderPassDX& ParallelRenderCommandListDX::GetPassDX()
{
    ITT_FUNCTION_TASK();
    return static_cast<RenderPassDX&>(GetPass());
}

void ParallelRenderCommandListDX::Execute(uint32_t frame_index)
{
    ITT_FUNCTION_TASK();
    
    ParallelRenderCommandListBase::Execute(frame_index);

    // NOTE: In DirectX there's no need for tracking command list completion, so it's completed right away
    Complete(frame_index);
}

} // namespace Methane::Graphics
