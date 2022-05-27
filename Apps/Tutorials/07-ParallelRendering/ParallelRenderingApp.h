/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: ParallelRenderingApp.h
Tutorial demonstrating parallel rendering with Methane graphics API

******************************************************************************/

#pragma once

#include <Methane/Kit.h>
#include <Methane/UserInterface/App.hpp>

namespace hlslpp // NOSONAR
{
#pragma pack(push, 16)
#include "Shaders/ParallelRenderingUniforms.h" // NOSONAR
#pragma pack(pop)
}

namespace Methane::Tutorials
{

namespace gfx = Methane::Graphics;

struct ParallelRenderingFrame final : Graphics::AppFrame
{
    gfx::InstancedMeshBufferBindings    cubes_array;
    Ptr<gfx::ParallelRenderCommandList> parallel_render_cmd_list_ptr;
    Ptr<gfx::CommandListSet>            execute_cmd_list_set_ptr;

    using gfx::AppFrame::AppFrame;
};

using UserInterfaceApp = UserInterface::App<ParallelRenderingFrame>;

class ParallelRenderingApp final : public UserInterfaceApp // NOSONAR
{
public:
    ParallelRenderingApp();
    ~ParallelRenderingApp() override;

    // GraphicsApp overrides
    void Init() override;
    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override;
    bool Update() override;
    bool Render() override;

protected:
    // IContextCallback override
    void OnContextReleased(gfx::Context& context) override;

private:
    struct CubeParameters
    {
        hlslpp::float4x4 model_matrix;
        double           rotation_speed_y = 0.25f;
        double           rotation_speed_z = 0.5f;
        uint32_t         thread_index = 0;
    };

    using CubeArrayParameters = std::vector<CubeParameters>;
    using MeshBuffers = gfx::MeshBuffers<hlslpp::Uniforms>;

    CubeArrayParameters InitializeCubeArrayParameters(uint32_t cubes_count, float base_cube_scale);
    bool Animate(double elapsed_seconds, double delta_seconds);

    gfx::Camera           m_camera;
    Ptr<gfx::RenderState> m_render_state_ptr;
    Ptr<gfx::Texture>     m_texture_array_ptr;
    Ptr<gfx::Sampler>     m_texture_sampler_ptr;
    Ptr<MeshBuffers>      m_cube_array_buffers_ptr;
    CubeArrayParameters   m_cube_array_parameters;

};

} // namespace Methane::Tutorials
