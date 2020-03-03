/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/DirectX12/ParallelRenderCommandListDX.h
DirectX 12 implementation of the render command list interface.

******************************************************************************/

#pragma once

#include "RenderPassDX.h"
#include "RenderCommandListDX.h"

#include <Methane/Graphics/ParallelRenderCommandListBase.h>

namespace Methane::Graphics
{

class CommandQueueDX;

class ParallelRenderCommandListDX final : public ParallelRenderCommandListBase
{
public:
    ParallelRenderCommandListDX(CommandQueueBase& cmd_buffer, RenderPassBase& render_pass);

    // ParallelRenderCommandList interface
    void Reset(const Ptr<RenderState>& sp_render_state, const std::string& debug_group = "") override;

    // CommandList interface
    void Commit() override;

    // CommandListBase interface
    void Execute(uint32_t frame_index) override;

    // Object interface
    void SetName(const std::string& name) override;

    using D3D12CommandLists = std::vector<ID3D12CommandList*>;
    D3D12CommandLists GetNativeCommandLists() const;

private:
    CommandQueueDX& GetCommandQueueDX();
    RenderPassDX&   GetPassDX();

    RenderCommandListDX m_beginning_command_list;
    RenderCommandListDX m_ending_command_list;
};

} // namespace Methane::Graphics
