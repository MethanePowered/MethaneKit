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

FILE: Methane/Graphics/RenderCommandListBase.h
Base implementation of the render command list interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RenderCommandList.h>
#include <Methane/Graphics/RenderPass.h>
#include <Methane/Graphics/RenderState.h>

#include "CommandListBase.h"
#include "RenderPassBase.h"

namespace Methane
{
namespace Graphics
{

struct RenderState;

class RenderCommandListBase
    : public RenderCommandList
    , public CommandListBase
{
public:
    using Ptr = std::shared_ptr<RenderCommandList>;

    RenderCommandListBase(CommandQueueBase& command_queue, RenderPassBase& render_pass);
    virtual ~RenderCommandListBase() override = default;

    // RenderCommandList interface
    virtual void Reset(RenderState& render_state, const std::string& debug_group = "") override;
    virtual void SetVertexBuffers(const Buffer::Refs& vertex_buffers) override;
    virtual void DrawIndexed(Primitive primitive_type, const Buffer& index_buffer, uint32_t instance_count) override;
    virtual void Draw(Primitive primitive_type, uint32_t vertex_count, uint32_t instance_count) override;

    RenderPassBase& GetPass();

protected:
    const RenderPass::Ptr m_sp_pass;
};

} // namespace Graphics
} // namespace Methane
