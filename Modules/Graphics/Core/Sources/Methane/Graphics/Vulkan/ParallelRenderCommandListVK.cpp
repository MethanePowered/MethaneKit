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

FILE: Methane/Graphics/Vulkan/ParallelRenderCommandListVK.mm
Vulkan implementation of the render command list interface.

******************************************************************************/

#include "ParallelRenderCommandListVK.h"
#include "RenderPassVK.h"
#include "CommandQueueVK.h"
#include "ContextVK.h"

#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

Ptr<ParallelRenderCommandList> ParallelRenderCommandList::Create(CommandQueue& command_queue, RenderPass& render_pass)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ParallelRenderCommandListVK>(static_cast<CommandQueueBase&>(command_queue), static_cast<RenderPassBase&>(render_pass));
}

ParallelRenderCommandListVK::ParallelRenderCommandListVK(CommandQueueBase& command_queue, RenderPassBase& render_pass)
    : ParallelRenderCommandListBase(command_queue, render_pass)
{
    ITT_FUNCTION_TASK();
}

void ParallelRenderCommandListVK::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    ParallelRenderCommandListBase::SetName(name);
}

void ParallelRenderCommandListVK::Reset(const Ptr<RenderState>& sp_render_state, const std::string& debug_group)
{
    ITT_FUNCTION_TASK();

    ParallelRenderCommandListBase::Reset(sp_render_state, debug_group);
}

void ParallelRenderCommandListVK::Commit()
{
    ITT_FUNCTION_TASK();
    
    assert(!IsCommitted());
    ParallelRenderCommandListBase::Commit();
}

void ParallelRenderCommandListVK::Execute(uint32_t frame_index)
{
    ITT_FUNCTION_TASK();

    ParallelRenderCommandListBase::Execute(frame_index);
}

CommandQueueVK& ParallelRenderCommandListVK::GetCommandQueueVK() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<class CommandQueueVK&>(GetCommandQueue());
}

RenderPassVK& ParallelRenderCommandListVK::GetPassVK()
{
    ITT_FUNCTION_TASK();
    return static_cast<class RenderPassVK&>(GetPass());
}

} // namespace Methane::Graphics
