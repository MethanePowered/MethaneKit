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
    using Ptr = std::shared_ptr<ParallelRenderCommandList>;

    ParallelRenderCommandListBase(CommandQueueBase& command_queue, RenderPassBase& render_pass);

    // CommandList interface
    void PushDebugGroup(const std::string& name) override   { throw std::logic_error("Not available for parallel render command list."); }
    void PopDebugGroup() override                           { throw std::logic_error("Not available for parallel render command list."); }

    // CommandListBase interface
    void SetResourceBarriers(const ResourceBase::Barriers&) override { }

    // ParallelRenderCommandList interface
    RenderCommandList::Ptrs CreateRenderCommandLists(uint32_t count) override;

    RenderPassBase& GetPass();

protected:
    const RenderPass::Ptr m_sp_pass;
};

} // namespace Methane::Graphics
