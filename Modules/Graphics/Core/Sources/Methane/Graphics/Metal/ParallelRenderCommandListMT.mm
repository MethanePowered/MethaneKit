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

FILE: Methane/Graphics/Metal/ParallelRenderCommandListMT.mm
Metal implementation of the render command list interface.

******************************************************************************/

#include "ParallelRenderCommandListMT.hh"
#include "RenderPassMT.hh"
#include "RenderStateMT.hh"
#include "CommandQueueMT.hh"
#include "RenderContextMT.hh"

#include <Methane/Instrumentation.h>
#include <Methane/Platform/MacOS/Types.hh>

namespace Methane::Graphics
{

Ptr<ParallelRenderCommandList> ParallelRenderCommandList::Create(CommandQueue& command_queue, RenderPass& render_pass)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ParallelRenderCommandListMT>(static_cast<CommandQueueBase&>(command_queue), static_cast<RenderPassBase&>(render_pass));
}

ParallelRenderCommandListMT::ParallelRenderCommandListMT(CommandQueueBase& command_queue, RenderPassBase& render_pass)
    : ParallelRenderCommandListBase(command_queue, render_pass)
{
    ITT_FUNCTION_TASK();
}

void ParallelRenderCommandListMT::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    ParallelRenderCommandListBase::SetName(name);
    
    NSString* ns_name = MacOS::ConvertToNsType<std::string, NSString*>(name);
    
    if (m_mtl_parallel_render_encoder != nil)
    {
        m_mtl_parallel_render_encoder.label = ns_name;
    }
    
    if (m_mtl_cmd_buffer != nil)
    {
        m_mtl_cmd_buffer.label = ns_name;
    }
}

void ParallelRenderCommandListMT::Reset(const Ptr<RenderState>& sp_render_state, const std::string& debug_group)
{
    ITT_FUNCTION_TASK();
    if (m_mtl_parallel_render_encoder != nil)
        return;

    // NOTE:
    //  If command buffer was not created for current frame yet,
    //  then render pass descriptor should be reset with new frame drawable
    RenderPassMT& render_pass = GetPassMT();
    MTLRenderPassDescriptor* mtl_render_pass = render_pass.GetNativeDescriptor(m_mtl_cmd_buffer == nil);

    if (m_mtl_cmd_buffer == nil)
    {
        m_mtl_cmd_buffer = [GetCommandQueueMT().GetNativeCommandQueue() commandBuffer];
        assert(m_mtl_cmd_buffer != nil);

        m_mtl_cmd_buffer.label = MacOS::ConvertToNsType<std::string, NSString*>(GetName());
    }

    assert(mtl_render_pass != nil);
    m_mtl_parallel_render_encoder = [m_mtl_cmd_buffer parallelRenderCommandEncoderWithDescriptor: mtl_render_pass];

    assert(m_mtl_parallel_render_encoder != nil);
    m_mtl_parallel_render_encoder.label = MacOS::ConvertToNsType<std::string, NSString*>(GetName());
    
    if (sp_render_state)
    {
        static_cast<RenderStateMT&>(*sp_render_state).InitializeNativeStates();
    }

    ParallelRenderCommandListBase::Reset(sp_render_state, debug_group);
}

void ParallelRenderCommandListMT::Commit()
{
    ITT_FUNCTION_TASK();
    
    assert(!IsCommitted());

    ParallelRenderCommandListBase::Commit();
    
    if (!m_mtl_cmd_buffer || !m_mtl_parallel_render_encoder)
        return;

    [m_mtl_parallel_render_encoder endEncoding];
    m_mtl_parallel_render_encoder = nil;

    [m_mtl_cmd_buffer enqueue];
}

void ParallelRenderCommandListMT::Execute(uint32_t frame_index)
{
    ITT_FUNCTION_TASK();

    ParallelRenderCommandListBase::Execute(frame_index);

    [m_mtl_cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer>) {
        Complete(frame_index);
    }];

    [m_mtl_cmd_buffer commit];
    m_mtl_cmd_buffer  = nil;
}

CommandQueueMT& ParallelRenderCommandListMT::GetCommandQueueMT() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<class CommandQueueMT&>(GetCommandQueue());
}

RenderPassMT& ParallelRenderCommandListMT::GetPassMT()
{
    ITT_FUNCTION_TASK();
    return static_cast<class RenderPassMT&>(GetPass());
}

} // namespace Methane::Graphics
