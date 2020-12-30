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

FILE: Methane/Graphics/Metal/ParallelRenderCommandListMT.mm
Metal implementation of the parallel render command list interface.

******************************************************************************/

#include "ParallelRenderCommandListMT.hh"
#include "RenderPassMT.hh"
#include "RenderStateMT.hh"
#include "RenderContextMT.hh"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<ParallelRenderCommandList> ParallelRenderCommandList::Create(CommandQueue& command_queue, RenderPass& render_pass)
{
    META_FUNCTION_TASK();
    return std::make_shared<ParallelRenderCommandListMT>(static_cast<CommandQueueBase&>(command_queue), static_cast<RenderPassBase&>(render_pass));
}

ParallelRenderCommandListMT::ParallelRenderCommandListMT(CommandQueueBase& command_queue, RenderPassBase& render_pass)
    : CommandListMT<id<MTLParallelRenderCommandEncoder>, ParallelRenderCommandListBase>(true, command_queue, render_pass)
{
    META_FUNCTION_TASK();
}

void ParallelRenderCommandListMT::Reset(DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    ResetCommandEncoder();
    ParallelRenderCommandListBase::Reset(p_debug_group);
}

void ParallelRenderCommandListMT::ResetWithState(RenderState& render_state, DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    if (ResetCommandEncoder())
    {
        static_cast<RenderStateMT&>(render_state.InitializeNativeStates();
    }
    ParallelRenderCommandListBase::ResetWithState(render_state, p_debug_group);
}

bool ParallelRenderCommandListMT::ResetCommandEncoder()
{
    META_FUNCTION_TASK();
    if (IsCommandEncoderInitialized())
        return false;

    // NOTE: If command buffer was not created for current frame yet,
    // then render pass descriptor should be reset with new frame drawable
    MTLRenderPassDescriptor* mtl_render_pass = GetRenderPassMT().GetNativeDescriptor(!IsCommandBufferInitialized());
    META_CHECK_ARG_NOT_NULL(mtl_render_pass);

    id<MTLCommandBuffer>& mtl_cmd_buffer = InitializeCommandBuffer();
    InitializeCommandEncoder([mtl_cmd_buffer parallelRenderCommandEncoderWithDescriptor: mtl_render_pass]);
    return true;
}

RenderPassMT& ParallelRenderCommandListMT::GetRenderPassMT()
{
    META_FUNCTION_TASK();
    return static_cast<class RenderPassMT&>(GetPass());
}

} // namespace Methane::Graphics
