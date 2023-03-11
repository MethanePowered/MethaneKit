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

FILE: Methane/Graphics/RHI/IComputeCommandList.h
Methane compute command list interface.

******************************************************************************/

#pragma once

#include "ICommandList.h"

#include <Methane/Graphics/Volume.hpp>
#include <Methane/Memory.hpp>

namespace Methane::Graphics::Rhi
{

struct IComputeState;

using ThreadGroupsCount = VolumeSize<uint32_t>;

struct IComputeCommandList
    : virtual ICommandList // NOSONAR
{
    static constexpr Type type = Type::Compute;

    // Create IComputeCommandList instance
    [[nodiscard]] static Ptr<IComputeCommandList> Create(ICommandQueue& command_queue);

    // IComputeCommandList interface
    virtual void ResetWithState(IComputeState& compute_state, IDebugGroup* debug_group_ptr = nullptr) = 0;
    virtual void ResetWithStateOnce(IComputeState& compute_state, IDebugGroup* debug_group_ptr = nullptr) = 0;
    virtual void SetComputeState(IComputeState& compute_state) = 0;
    virtual void Dispatch(const ThreadGroupsCount& thread_groups_count) = 0;
};

} // namespace Methane::Graphics::Rhi
