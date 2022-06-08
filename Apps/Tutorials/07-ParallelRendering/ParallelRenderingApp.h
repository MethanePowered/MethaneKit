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
    Ptr<gfx::RenderCommandList>         serial_render_cmd_list_ptr;
    Ptr<gfx::CommandListSet>            execute_cmd_list_set_ptr;

    using gfx::AppFrame::AppFrame;
};

using UserInterfaceApp = UserInterface::App<ParallelRenderingFrame>;

class ParallelRenderingApp final : public UserInterfaceApp // NOSONAR
{
public:
    struct Settings
    {
        uint32_t cubes_grid_size            = 12U; // total_cubes_count = pow(cubes_grid_size, 3)
        uint32_t render_thread_count        = std::thread::hardware_concurrency();
        bool     parallel_rendering_enabled = true;

        bool operator==(const Settings& other) const noexcept;

        uint32_t GetTotalCubesCount() const noexcept;
        uint32_t GetRenderThreadCount() const noexcept;
    };

    ParallelRenderingApp();
    ~ParallelRenderingApp() override;

    // GraphicsApp overrides
    void Init() override;
    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override;
    bool Update() override;
    bool Render() override;

    // UserInterface::App overrides
    std::string GetParametersString() override;

    const Settings& GetSettings() const noexcept { return m_settings; }
    void SetSettings(const Settings& settings);

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

    CubeArrayParameters InitializeCubeArrayParameters();
    bool Animate(double elapsed_seconds, double delta_seconds);
    void RenderCubesRange(gfx::RenderCommandList& remder_cmd_list, const Ptrs<gfx::ProgramBindings>& program_bindings_per_instance,
                          uint32_t begin_instance_index, const uint32_t end_instance_index);

    Settings              m_settings;
    gfx::Camera           m_camera;
    Ptr<gfx::RenderState> m_render_state_ptr;
    Ptr<gfx::Texture>     m_texture_array_ptr;
    Ptr<gfx::Sampler>     m_texture_sampler_ptr;
    Ptr<MeshBuffers>      m_cube_array_buffers_ptr;
    CubeArrayParameters   m_cube_array_parameters;

};

} // namespace Methane::Tutorials
