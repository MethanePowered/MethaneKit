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

static std::string GetParallelCommandListDebugName(const std::string& base_name, const std::string& suffix)
{
    return base_name.empty() ? std::string() : base_name + " " + suffix;
}

static std::string GetTrailingCommandListDebugName(const std::string& base_name, bool is_beginning)
{
    return GetParallelCommandListDebugName(base_name, is_beginning ? "[Beginning]" : "[Ending]");
}

ParallelRenderCommandList::Ptr ParallelRenderCommandList::Create(CommandQueue& cmd_queue, RenderPass& render_pass)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ParallelRenderCommandListDX>(static_cast<CommandQueueBase&>(cmd_queue), static_cast<RenderPassBase&>(render_pass));
}

ParallelRenderCommandListDX::ParallelRenderCommandListDX(CommandQueueBase& cmd_buffer, RenderPassBase& render_pass)
    : ParallelRenderCommandListBase(cmd_buffer, render_pass)
    , m_begining_command_list(cmd_buffer, render_pass)
    , m_ending_command_list(cmd_buffer, render_pass)
{
    ITT_FUNCTION_TASK();

    // Native D3D12 render-pass usage is disabled to do the render target setup and clears
    // in "begin" command list once before parallel rendering
    GetPassDX().SetNativeRenderPassUsage(false);

    const wrl::ComPtr<ID3D12Device>& cp_device = GetCommandQueueDX().GetContextDX().GetDeviceDX().GetNativeDevice();
    assert(!!cp_device);
}

void ParallelRenderCommandListDX::Reset(const RenderState::Ptr& sp_render_state, const std::string& debug_group)
{
    ITT_FUNCTION_TASK();

    // Render pass is begun in "beginning" command list only,
    // but it will be ended in the "ending" command list on commit of the parallel CL
    m_begining_command_list.Reset(RenderState::Ptr(), debug_group); // begins render pass
    m_ending_command_list.ResetNative();                            // only reset native command list

    // Instead of closing debug group (from Reset call) on beginning CL commit, we force to close it in ending CL
    m_begining_command_list.SetDebugGroupOpened(false);             
    m_ending_command_list.SetDebugGroupOpened(true);

    ParallelRenderCommandListBase::Reset(sp_render_state, debug_group);
}

void ParallelRenderCommandListDX::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    m_begining_command_list.SetName(GetTrailingCommandListDebugName(name, true));
    m_ending_command_list.SetName(GetTrailingCommandListDebugName(name, false));

    ParallelRenderCommandListBase::SetName(name);
}

void ParallelRenderCommandListDX::Commit(bool present_drawable)
{
    ITT_FUNCTION_TASK();

    // Render pass was begun in "beginning" command list,
    // but it is ended in "ending" command list only
    m_ending_command_list.Commit(false);    // ends render pass
    m_begining_command_list.Commit(false);

    ParallelRenderCommandListBase::Commit(present_drawable);
}

void ParallelRenderCommandListDX::Execute(uint32_t frame_index)
{
    ITT_FUNCTION_TASK();

    m_begining_command_list.Execute(frame_index);
    
    ParallelRenderCommandListBase::Execute(frame_index);

    m_ending_command_list.Execute(frame_index);

    // NOTE: In DirectX there's no need for tracking command list completion, so it's completed right away
    CommandListBase::Complete(frame_index);
}

ParallelRenderCommandListDX::D3D12CommandLists ParallelRenderCommandListDX::GetNativeCommandLists() const
{
    ITT_FUNCTION_TASK();

    D3D12CommandLists dx_command_lists;
    dx_command_lists.reserve(m_parallel_comand_lists.size() + 2); // 2 command lists reserved for beginning and ending
    dx_command_lists.push_back(m_begining_command_list.GetNativeCommandList().Get());

    for (const RenderCommandList::Ptr& sp_command_list : m_parallel_comand_lists)
    {
        assert(!!sp_command_list);
        RenderCommandListDX& dx_command_list = static_cast<RenderCommandListDX&>(*sp_command_list);
        dx_command_lists.push_back(dx_command_list.GetNativeCommandList().Get());
    }

    dx_command_lists.push_back(m_ending_command_list.GetNativeCommandList().Get());
    return dx_command_lists;
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

} // namespace Methane::Graphics
