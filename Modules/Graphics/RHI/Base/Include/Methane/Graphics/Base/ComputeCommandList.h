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

FILE: Methane/Graphics/Base/ComputeCommandList.h
Base implementation of the compute command list interface.

******************************************************************************/

#pragma once

#include "CommandList.h"

#include <Methane/Graphics/RHI/IComputeCommandList.h>

namespace Methane::Graphics::Base
{

class ComputeState;

class ComputeCommandList
    : public Rhi::IComputeCommandList
    , public CommandList
{
public:
    explicit ComputeCommandList(CommandQueue& command_queue);

    using CommandList::Reset;

    // IComputeCommandList interface
    void ResetWithState(Rhi::IComputeState& compute_state, IDebugGroup* debug_group_ptr = nullptr) final;
    void ResetWithStateOnce(Rhi::IComputeState& compute_state, IDebugGroup* debug_group_ptr = nullptr) final;
    void SetComputeState(Rhi::IComputeState& compute_state) final;
    void Dispatch(const Rhi::ThreadGroupsCount& thread_groups_count) override;

    ComputeState& GetComputeState();

private:
    Ptr<ComputeState> m_compute_state_ptr;
};

} // namespace Methane::Graphics::Base
