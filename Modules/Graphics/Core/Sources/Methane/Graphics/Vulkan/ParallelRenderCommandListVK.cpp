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

FILE: Methane/Graphics/Vulkan/ParallelRenderCommandListVK.cpp
Vulkan implementation of the render command list interface.

******************************************************************************/

#include "ParallelRenderCommandListVK.h"
#include "RenderPassVK.h"
#include "CommandQueueVK.h"
#include "ContextVK.h"

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

Ptr<ParallelRenderCommandList> ParallelRenderCommandList::Create(CommandQueue& command_queue, RenderPass& render_pass)
{
    META_FUNCTION_TASK();
#if 0
    return std::make_shared<ParallelRenderCommandListVK>(static_cast<CommandQueueBase&>(command_queue), static_cast<RenderPassBase&>(render_pass));
#else
    META_UNUSED(command_queue);
    META_UNUSED(render_pass);
    META_FUNCTION_NOT_IMPLEMENTED_DESCR("ParallelRenderCommandList has no Vulkan API implementation yet");
#endif
}

ParallelRenderCommandListVK::ParallelRenderCommandListVK(CommandQueueBase& command_queue, RenderPassBase& render_pass)
    : ParallelRenderCommandListBase(command_queue, render_pass)
{
    META_FUNCTION_TASK();
}

void ParallelRenderCommandListVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    ParallelRenderCommandListBase::SetName(name);
}

void ParallelRenderCommandListVK::Reset(DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    ParallelRenderCommandListBase::Reset(p_debug_group);
}

void ParallelRenderCommandListVK::ResetWithState(RenderState& render_state, DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    ParallelRenderCommandListBase::ResetWithState(render_state, p_debug_group);
}

void ParallelRenderCommandListVK::SetBeginningResourceBarriers(const Resource::Barriers&)
{
    META_FUNCTION_TASK();
    META_FUNCTION_NOT_IMPLEMENTED();
}

void ParallelRenderCommandListVK::SetEndingResourceBarriers(const Resource::Barriers&)
{
    META_FUNCTION_TASK();
    META_FUNCTION_NOT_IMPLEMENTED();
}

void ParallelRenderCommandListVK::Commit()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_FALSE(IsCommitted());
    ParallelRenderCommandListBase::Commit();
}

void ParallelRenderCommandListVK::Execute(uint32_t frame_index, const CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    ParallelRenderCommandListBase::Execute(frame_index, completed_callback);
}

CommandQueueVK& ParallelRenderCommandListVK::GetCommandQueueVK() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<class CommandQueueVK&>(GetCommandQueue());
}

RenderPassVK& ParallelRenderCommandListVK::GetPassVK()
{
    META_FUNCTION_TASK();
    return static_cast<class RenderPassVK&>(GetPass());
}

} // namespace Methane::Graphics
