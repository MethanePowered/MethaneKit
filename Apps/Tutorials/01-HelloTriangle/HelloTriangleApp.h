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

FILE: HelloTriangleApp.cpp
Tutorial demonstrating triangle rendering with Methane graphics API

******************************************************************************/

#pragma once

#include <Methane/Graphics/Kit.h>

#include <string>
#include <array>

namespace Methane::Tutorials
{

namespace gfx = Methane::Graphics;

struct HelloTriangleFrame final : gfx::AppFrame
{
    gfx::RenderCommandList::Ptr sp_cmd_list;

    using gfx::AppFrame::AppFrame;
};

using GraphicsApp = gfx::App<HelloTriangleFrame>;

class HelloTriangleApp final : public GraphicsApp
{
public:
    HelloTriangleApp();
    ~HelloTriangleApp() override;

    // App interface
    void Init() override;
    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override;
    void Render() override;

    // Context::Callback interface
    void OnContextReleased() override;

private:
    struct Vertex
    {
        gfx::Vector3f position;
        gfx::Vector3f color;
    };

    using Vertices = std::array<Vertex, 3>;
    const Vertices m_triangle_vertices;

    gfx::Program::Ptr       m_sp_program;
    gfx::RenderState::Ptr   m_sp_state;
    gfx::Buffer::Ptr        m_sp_vertex_buffer;
};

} // namespace Methane::Tutorials
