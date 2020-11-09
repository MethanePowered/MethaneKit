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

FILE: Methane/Graphics/ParallelRenderCommandList.h
Methane parallel render command list interface
for multi-threaded rendering in the single render pass.

******************************************************************************/

#pragma once

#include "RenderCommandList.h"

#include <Methane/Memory.hpp>

namespace Methane::Graphics
{

struct RenderState;
struct RenderPass;

struct ParallelRenderCommandList : virtual CommandList
{
    // Create ParallelRenderCommandList instance
    static Ptr<ParallelRenderCommandList> Create(CommandQueue& command_queue, RenderPass& render_pass);
    
    // ParallelRenderCommandList interface
    virtual bool IsValidationEnabled() const noexcept = 0;
    virtual void SetValidationEnabled(bool is_validation_enabled) = 0;
    virtual void ResetWithState(const Ptr<RenderState>& render_state_ptr = nullptr, DebugGroup* p_debug_group = nullptr) = 0;
    virtual void SetViewState(ViewState& view_state) = 0;
    virtual void SetParallelCommandListsCount(uint32_t count) = 0;
    virtual const Ptrs<RenderCommandList>& GetParallelCommandLists() const = 0;
    
    using CommandList::Reset;
};

} // namespace Methane::Graphics
