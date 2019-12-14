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

FILE: Methane/Graphics/ParallelRenderCommandListBase.cpp
Base implementation of the parallel render command list interface.

******************************************************************************/

#include "ParallelRenderCommandListBase.h"
#include "RenderCommandListBase.h"
#include "RenderPassBase.h"
#include "RenderStateBase.h"
#include "BufferBase.h"
#include "ProgramBase.h"

#include <Methane/Data/Instrumentation.h>
#include <Methane/Data/Parallel.hpp>

#include <cassert>

namespace Methane::Graphics
{

ParallelRenderCommandListBase::ParallelRenderCommandListBase(CommandQueueBase& command_queue, RenderPassBase& pass)
    : CommandListBase(command_queue)
    , m_sp_pass(pass.GetPtr())
{
    ITT_FUNCTION_TASK();
}

void ParallelRenderCommandListBase::Reset(RenderState& render_state)
{
    ITT_FUNCTION_TASK();

    Data::ParallelFor<RenderCommandList::Ptrs::const_iterator, RenderCommandList::Ptr>(m_parallel_comand_lists.begin(), m_parallel_comand_lists.end(),
        [&render_state](const RenderCommandList::Ptr& sp_render_command_list, Data::Index cl_index)
        {
            assert(sp_render_command_list);
            sp_render_command_list->Reset(render_state);
        });
}

void ParallelRenderCommandListBase::Commit(bool present_drawable)
{
    ITT_FUNCTION_TASK();

    Data::ParallelFor<RenderCommandList::Ptrs::const_iterator, RenderCommandList::Ptr>(m_parallel_comand_lists.begin(), m_parallel_comand_lists.end(),
        [present_drawable](const RenderCommandList::Ptr& sp_render_command_list, Data::Index cl_index)
        {
            assert(sp_render_command_list);
            sp_render_command_list->Commit(present_drawable);
        });
}

void ParallelRenderCommandListBase::SetParallelCommandListsCount(uint32_t count)
{
    ITT_FUNCTION_TASK();

    uint32_t initial_count = static_cast<uint32_t>(m_parallel_comand_lists.size());
    if (count < initial_count)
    {
        m_parallel_comand_lists.resize(count);
        return;
    }

    m_parallel_comand_lists.reserve(count);
    for(uint32_t cl_index = initial_count; cl_index < count; ++cl_index)
    {
        m_parallel_comand_lists.emplace_back(RenderCommandList::Create(*this));
    }
}

RenderPassBase& ParallelRenderCommandListBase::GetPass()
{
    ITT_FUNCTION_TASK();
    assert(!!m_sp_pass);
    return static_cast<RenderPassBase&>(*m_sp_pass);
}

} // namespace Methane::Graphics
