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

FILE: Methane/Graphics/ParallelRenderCommandListBase.h
Base implementation of the parallel render command list interface.

******************************************************************************/

#pragma once

#include "CommandListBase.h"
#include "RenderPassBase.h"

#include <Methane/Graphics/ParallelRenderCommandList.h>

#include <optional>

namespace Methane::Graphics
{

struct RenderState;

class ParallelRenderCommandListBase
    : public ParallelRenderCommandList
    , public CommandListBase
{
public:
    ParallelRenderCommandListBase(CommandQueueBase& command_queue, RenderPassBase& render_pass);
    
    using CommandListBase::Reset;

    // ParallelRenderCommandList interface
    void Reset(const Ptr<RenderState>& sp_render_state, const std::string& debug_group = "") override;
    void SetParallelCommandListsCount(uint32_t count) override;
    const Ptrs<RenderCommandList>& GetParallelCommandLists() const override { return m_parallel_command_lists; }

    // CommandListBase interface
    void SetResourceBarriers(const ResourceBase::Barriers&) override { throw std::logic_error("Can not set resource barriers on parallel render command list."); }
    void Execute(uint32_t frame_index) override;
    void Complete(uint32_t frame_index) override;

    // CommandList interface
    void PushDebugGroup(const std::string& name) override   { throw std::logic_error("Can no use debug groups on parallel render command list."); }
    void PopDebugGroup() override                           { throw std::logic_error("Can no use debug groups on parallel render command list."); }
    void Commit() override;

    // Object interface
    void SetName(const std::string& name) override;

    RenderPassBase& GetPass();

private:
    const Ptr<RenderPass>   m_sp_pass;
    Ptrs<RenderCommandList> m_parallel_command_lists;
};

} // namespace Methane::Graphics
