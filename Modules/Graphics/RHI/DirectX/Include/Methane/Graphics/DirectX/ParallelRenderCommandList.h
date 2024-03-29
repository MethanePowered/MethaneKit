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

FILE: Methane/Graphics/DirectX/ParallelRenderCommandList.h
DirectX 12 implementation of the render command list interface.

******************************************************************************/

#pragma once

#include "RenderPass.h"
#include "RenderCommandList.h"

#include <Methane/Graphics/Base/ParallelRenderCommandList.h>

namespace Methane::Graphics::DirectX
{

class CommandQueue;

class ParallelRenderCommandList final : public Base::ParallelRenderCommandList
{
public:
    ParallelRenderCommandList(Base::CommandQueue& cmd_queue, Base::RenderPass& render_pass);

    // IParallelRenderCommandList interface
    void ResetWithState(Rhi::IRenderState& render_state, IDebugGroup* debug_group_ptr = nullptr) override;
    void SetBeginningResourceBarriers(const Rhi::IResourceBarriers& resource_barriers) override;
    void SetEndingResourceBarriers(const Rhi::IResourceBarriers& resource_barriers) override;

    // ICommandList interface
    void Commit() override;

    // Base::CommandList interface
    void Execute(const CompletedCallback& completed_callback = {}) override;
    void Complete() override;

    // IObject interface
    bool SetName(std::string_view name) override;

    using D3D12CommandLists = std::vector<ID3D12CommandList*>;
    D3D12CommandLists GetNativeCommandLists() const;

protected:
    // ParallelRenderCommandListBase interface
    [[nodiscard]] Ptr<Rhi::IRenderCommandList> CreateCommandList(bool is_beginning_list) override;

private:
    CommandQueue& GetDirectCommandQueue();
    RenderPass&   GetDirectPass();

    RenderCommandList m_beginning_command_list;
    RenderCommandList m_ending_command_list;
};

} // namespace Methane::Graphics::DirectX
