/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Base/ComputeCommandList.cpp
Base implementation of the compute command list interface.

******************************************************************************/

#include <Methane/Graphics/Base/ComputeCommandList.h>
#include <Methane/Graphics/Base/ComputeState.h>
#include <Methane/Graphics/Base/CommandQueue.h>
#include <Methane/Graphics/Base/Program.h>
#include <Methane/Graphics/TypeFormatters.hpp>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Base
{

ComputeCommandList::ComputeCommandList(CommandQueue& command_queue)
    : CommandList(command_queue, Type::Compute)
{ }

void ComputeCommandList::ResetWithState(Rhi::IComputeState& compute_state, IDebugGroup* debug_group_ptr)
{
    META_FUNCTION_TASK();
    Reset(debug_group_ptr);
    SetComputeState(compute_state);
}

void ComputeCommandList::ResetWithStateOnce(Rhi::IComputeState& compute_state, IDebugGroup* debug_group_ptr)
{
    META_FUNCTION_TASK();
    if (GetState() == State::Encoding && m_compute_state_ptr.get() == std::addressof(compute_state))
    {
        META_LOG("{} Command list '{}' was already RESET with the same compute state '{}'", magic_enum::enum_name(GetType()), GetName(), compute_state.GetName());
        return;
    }
    ResetWithState(compute_state, debug_group_ptr);
}

void ComputeCommandList::SetComputeState(Rhi::IComputeState& compute_state)
{
    META_FUNCTION_TASK();
    META_LOG("{} Command list '{}' SET COMPUTE STATE '{}':\n{}", magic_enum::enum_name(GetType()), GetName(), compute_state.GetName(), static_cast<std::string>(compute_state.GetSettings()));

    VerifyEncodingState();

    const bool render_state_changed = m_compute_state_ptr.get() != std::addressof(compute_state);
    auto& compute_state_base = static_cast<ComputeState&>(compute_state);
    compute_state_base.Apply(*this);

    Ptr<Object> compute_state_object_ptr = compute_state_base.GetBasePtr();
    m_compute_state_ptr = std::static_pointer_cast<ComputeState>(compute_state_object_ptr);

    if (render_state_changed)
    {
        RetainResource(compute_state_object_ptr);
    }
}

ComputeState& ComputeCommandList::GetComputeState()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(m_compute_state_ptr, "Compute command list '{}' state was not set.", GetName());
    return *m_compute_state_ptr;
}

void ComputeCommandList::Dispatch([[maybe_unused]] const Rhi::ThreadGroupsCount& thread_groups_count)
{
    META_FUNCTION_TASK();
    META_LOG("{} Command list '{}' DISPATCH {} thread groups count.",
             magic_enum::enum_name(GetType()), GetName(), thread_groups_count);
}

} // namespace Methane::Graphics::Base
