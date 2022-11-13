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

FILE: Methane/Graphics/DirectX/ParallelRenderCommandList.cpp
DirectX 12 implementation of the parallel render command list interface.

******************************************************************************/

#include <Methane/Graphics/DirectX/ParallelRenderCommandList.h>
#include <Methane/Graphics/DirectX/RenderState.h>
#include <Methane/Graphics/DirectX/RenderPass.h>
#include <Methane/Graphics/DirectX/CommandQueue.h>
#include <Methane/Graphics/DirectX/Device.h>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Instrumentation.h>

#include <directx/d3dx12.h>

#include <fmt/format.h>
#include <string_view>

namespace Methane::Graphics::Rhi
{

Ptr<IParallelRenderCommandList> Rhi::IParallelRenderCommandList::Create(ICommandQueue& cmd_queue, Rhi::IRenderPass& render_pass)
{
    META_FUNCTION_TASK();
    return std::make_shared<DirectX::ParallelRenderCommandList>(static_cast<Base::CommandQueue&>(cmd_queue), static_cast<Base::RenderPass&>(render_pass));
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::DirectX
{

ParallelRenderCommandList::ParallelRenderCommandList(Base::CommandQueue& cmd_queue, Base::RenderPass& render_pass)
    : Base::ParallelRenderCommandList(cmd_queue, render_pass)
    , m_beginning_command_list(cmd_queue, render_pass)
    , m_ending_command_list(cmd_queue, render_pass)
{
    META_FUNCTION_TASK();

    // Native D3D12 render-pass usage is disabled to do the render target setup and clears
    // in "begin" command list once before parallel rendering
    GetDirectPass().SetNativeRenderPassUsage(false);
}

void ParallelRenderCommandList::ResetWithState(IRenderState& render_state, Rhi::IDebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();

    // Render pass is begun in "beginning" command list only,
    // but it will be ended in the "ending" command list on commit of the parallel CL
    m_beginning_command_list.Reset(p_debug_group); // begins render pass
    m_ending_command_list.ResetNative();           // only reset native command list

    // Instead of closing debug group (from Reset call) on beginning CL commit, we force to close it in ending CL
    if (p_debug_group)
    {
        m_beginning_command_list.ClearOpenDebugGroups();
        m_ending_command_list.PushOpenDebugGroup(*p_debug_group);
    }

    // Initialize native pipeline state before resetting per-thread command lists
    // to allow parallel reset of all CLs at once with using native pipeline state for each reset
    auto& dx_render_state = static_cast<RenderState&>(render_state);
    dx_render_state.InitializeNativePipelineState();

    Base::ParallelRenderCommandList::ResetWithState(render_state, p_debug_group);
}

void ParallelRenderCommandList::SetBeginningResourceBarriers(const Rhi::IResourceBarriers& resource_barriers)
{
    META_FUNCTION_TASK();
    m_beginning_command_list.SetResourceBarriers(resource_barriers);
}

void ParallelRenderCommandList::SetEndingResourceBarriers(const Rhi::IResourceBarriers& resource_barriers)
{
    META_FUNCTION_TASK();
    m_ending_command_list.SetResourceBarriers(resource_barriers);
}

bool ParallelRenderCommandList::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!Base::ParallelRenderCommandList::SetName(name))
        return false;

    m_beginning_command_list.SetName(GetTrailingCommandListDebugName(name, true));
    m_ending_command_list.SetName(GetTrailingCommandListDebugName(name, false));
    return true;
}

void ParallelRenderCommandList::Commit()
{
    META_FUNCTION_TASK();
    Base::ParallelRenderCommandList::Commit();

    // Render pass was begun in "beginning" command list,
    // but it is ended in "ending" command list only
    m_ending_command_list.Commit();    // ends render pass
    m_beginning_command_list.Commit();
}

void ParallelRenderCommandList::Execute(const Rhi::ICommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    m_beginning_command_list.Execute();
    
    Base::ParallelRenderCommandList::Execute(completed_callback);

    m_ending_command_list.Execute();
}

void ParallelRenderCommandList::Complete()
{
    META_FUNCTION_TASK();
    m_beginning_command_list.Complete();

    Base::ParallelRenderCommandList::Complete();

    m_ending_command_list.Complete();
}

ParallelRenderCommandList::D3D12CommandLists ParallelRenderCommandList::GetNativeCommandLists() const
{
    META_FUNCTION_TASK();
    D3D12CommandLists dx_command_lists;
    const Refs<IRenderCommandList>& parallel_command_lists = GetParallelCommandLists();
    dx_command_lists.reserve(parallel_command_lists.size() + 2); // 2 command lists reserved for beginning and ending
    dx_command_lists.push_back(&m_beginning_command_list.GetNativeCommandList());

    for (const Ref<IRenderCommandList>& command_list_ref : parallel_command_lists)
    {
        dx_command_lists.push_back(&static_cast<const RenderCommandList&>(command_list_ref.get()).GetNativeCommandList());
    }

    dx_command_lists.push_back(&m_ending_command_list.GetNativeCommandList());
    return dx_command_lists;
}

CommandQueue& ParallelRenderCommandList::GetDirectCommandQueue()
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueue&>(GetBaseCommandQueue());
}

RenderPass& ParallelRenderCommandList::GetDirectPass()
{
    META_FUNCTION_TASK();
    return static_cast<RenderPass&>(GetPass());
}

} // namespace Methane::Graphics::DirectX
