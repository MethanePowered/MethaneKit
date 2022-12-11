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

FILE: Methane/Graphics/Metal/ParallelRenderCommandList.hh
Metal implementation of the parallel render command list interface.

******************************************************************************/

#pragma once

#include "CommandList.hpp"

#include <Methane/Graphics/Base/ParallelRenderCommandList.h>

#import <Metal/Metal.h>

namespace Methane::Graphics::Metal
{

class CommandQueue;
class Buffer;
class RenderPass;

class ParallelRenderCommandList final
    : public CommandList<id<MTLParallelRenderCommandEncoder>, Base::ParallelRenderCommandList>
{
public:
    ParallelRenderCommandList(Base::CommandQueue& command_queue, Base::RenderPass& render_pass);

    // IParallelRenderCommandList interface
    void Reset(IDebugGroup* debug_group_ptr = nullptr) override;
    void ResetWithState(Rhi::IRenderState& render_state, IDebugGroup* debug_group_ptr = nullptr) override;
    void SetBeginningResourceBarriers(const Rhi::IResourceBarriers&) override { }
    void SetEndingResourceBarriers(const Rhi::IResourceBarriers&) override { }

private:
    RenderPass& GetMetalRenderPass();
    bool ResetCommandEncoder();
};

} // namespace Methane::Graphics::Metal
