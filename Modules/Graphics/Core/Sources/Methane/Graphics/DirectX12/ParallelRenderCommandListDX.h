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

FILE: Methane/Graphics/DirectX12/ParallelRenderCommandListDX.h
DirectX 12 implementation of the render command list interface.

******************************************************************************/

#pragma once

#include "RenderPassDX.h"

#include <Methane/Graphics/ParallelRenderCommandListBase.h>

#include <wrl.h>
#include <d3d12.h>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

class CommandQueueDX;
class RenderPassDX;

class ParallelRenderCommandListDX final : public ParallelRenderCommandListBase
{
public:
    ParallelRenderCommandListDX(CommandQueueBase& cmd_buffer, RenderPassBase& render_pass);

    // ParallelRenderCommandList interface
    void Reset(RenderState& render_state) override;

    // CommandList interface
    void Commit(bool present_drawable) override;

    // CommandListBase interface
    void Execute(uint32_t frame_index) override;

    // Object interface
    void SetName(const std::string& name) override;

protected:
    CommandQueueDX& GetCommandQueueDX();
    RenderPassDX&   GetPassDX();

    wrl::ComPtr<ID3D12CommandAllocator>       m_cp_command_allocator;
    bool                                      m_is_committed = false;
};

} // namespace Methane::Graphics
