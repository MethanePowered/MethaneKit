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

#include <cassert>

namespace Methane::Graphics
{

ParallelRenderCommandListBase::ParallelRenderCommandListBase(CommandQueueBase& command_queue, RenderPassBase& pass)
    : CommandListBase(command_queue)
    , m_sp_pass(pass.GetPtr())
{
    ITT_FUNCTION_TASK();
}

RenderPassBase& ParallelRenderCommandListBase::GetPass()
{
    ITT_FUNCTION_TASK();
    assert(!!m_sp_pass);
    return static_cast<RenderPassBase&>(*m_sp_pass);
}

RenderCommandList::Ptrs ParallelRenderCommandListBase::CreateRenderCommandLists(uint32_t count)
{
    ITT_FUNCTION_TASK();
    RenderCommandList::Ptrs render_command_lists(count);
    for(RenderCommandList::Ptr& sp_render_command_list : render_command_lists)
    {
        sp_render_command_list = RenderCommandList::Create(*this);
    }
    return render_command_lists;
}

} // namespace Methane::Graphics
