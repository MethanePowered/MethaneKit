/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: HelloCubeApp.cpp
Tutorial demonstrating colored cube rendering with Methane graphics API

******************************************************************************/

#pragma once

#include <Methane/Kit.h>
#include <Methane/Graphics/App.hpp>
#include <Methane/Graphics/Mesh/CubeMesh.hpp>

namespace Methane::Tutorials
{

namespace gfx = Methane::Graphics;

struct HelloCubeFrame final : gfx::AppFrame
{
    Ptr<gfx::BufferSet>         vertex_buffer_set_ptr;
    Ptr<gfx::RenderCommandList> render_cmd_list_ptr;
    Ptr<gfx::CommandListSet>    execute_cmd_list_set_ptr;

    using gfx::AppFrame::AppFrame;
};

using GraphicsApp = gfx::App<HelloCubeFrame>;
class HelloCubeApp final : public GraphicsApp
{
public:
    HelloCubeApp();
    ~HelloCubeApp() override;

    // GraphicsApp overrides
    void Init() override;
    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override;
    bool Update() override;
    bool Render() override;

    // IContextCallback interface
    void OnContextReleased(gfx::Context& context) override;

private:
    bool Animate(double elapsed_seconds, double delta_seconds);

    struct CubeVertex
    {
        gfx::Mesh::Position position;
        gfx::Mesh::Color    color;

        inline static const gfx::Mesh::VertexLayout layout{
            gfx::Mesh::VertexField::Position,
            gfx::Mesh::VertexField::Color
        };
    };

    const gfx::CubeMesh<CubeVertex> m_cube_mesh;
    std::vector<CubeVertex>         m_proj_vertices;
    gfx::Camera                     m_camera;
    hlslpp::float4x4                m_model_matrix;

    Ptr<gfx::RenderState>   m_render_state_ptr;
    Ptr<gfx::Buffer>        m_index_buffer_ptr;
};

} // namespace Methane::Tutorials
