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

FILE: Methane/Graphics/IParallelRenderCommandList.h
Methane parallel render command list interface
for multi-threaded rendering in the single render pass.

******************************************************************************/

#pragma once

#include "IRenderCommandList.h"

#include <Methane/Memory.hpp>

namespace Methane::Graphics
{

struct IRenderState;
struct IRenderPass;

struct IParallelRenderCommandList
    : virtual ICommandList // NOSONAR
{
    static constexpr Type type = Type::ParallelRender;

    // Create IParallelRenderCommandList instance
    [[nodiscard]] static Ptr<IParallelRenderCommandList> Create(ICommandQueue& command_queue, IRenderPass& render_pass);
    
    // IParallelRenderCommandList interface
    [[nodiscard]] virtual bool IsValidationEnabled() const noexcept = 0;
    virtual void SetValidationEnabled(bool is_validation_enabled) = 0;
    virtual void ResetWithState(IRenderState& render_state, IDebugGroup* p_debug_group = nullptr) = 0;
    virtual void SetViewState(IViewState& view_state) = 0;
    virtual void SetBeginningResourceBarriers(const IResourceBarriers& resource_barriers) = 0;
    virtual void SetEndingResourceBarriers(const IResourceBarriers& resource_barriers) = 0;
    virtual void SetParallelCommandListsCount(uint32_t count) = 0;
    [[nodiscard]] virtual const Refs<IRenderCommandList>& GetParallelCommandLists() const = 0;
    
    using ICommandList::Reset;
};

} // namespace Methane::Graphics
