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

FILE: Methane/Graphics/RenderCommandList.h
Methane render command list interface.

******************************************************************************/

#pragma once

#include "CommandList.h"
#include "Buffer.h"

namespace Methane::Graphics
{

struct RenderState;
struct RenderPass;

struct RenderCommandList : virtual CommandList
{
    using Ptr = std::shared_ptr<RenderCommandList>;

    enum class Primitive
    {
        Point,
        Line,
        LineStrip,
        Triangle,
        TriangleStrip
    };

    // Create RenderCommandList instance
    static Ptr Create(CommandQueue& command_queue, RenderPass& render_pass);

    // RenderCommandList interface
    virtual void Reset(RenderState& render_state, const std::string& debug_group = "") = 0;
    virtual void SetVertexBuffers(const Buffer::Refs& vertex_buffers) = 0;
    virtual void DrawIndexed(Primitive primitive, const Buffer& index_buffer, 
                             uint32_t index_count = 0, uint32_t start_index = 0, uint32_t start_vertex = 0, 
                             uint32_t instance_count = 1, uint32_t start_instance = 0) = 0;
    virtual void Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex = 0,
                      uint32_t instance_count = 1, uint32_t start_instance = 0) = 0;
};

} // namespace Methane::Graphics
