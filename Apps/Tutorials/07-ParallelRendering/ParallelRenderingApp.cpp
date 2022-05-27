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

FILE: ParallelRenderingApp.cpp
Tutorial demonstrating parallel rendering with Methane graphics API

******************************************************************************/

#include "ParallelRenderingApp.h"

#include <Methane/Samples/TextureLabeler.h>
#include <Methane/Samples/AppSettings.hpp>
#include <Methane/Graphics/CubeMesh.hpp>
#include <Methane/Data/TimeAnimation.h>

#include <taskflow/taskflow.hpp>
#include <magic_enum.hpp>
#include <cmath>
#include <random>
#include <algorithm>

namespace Methane::Tutorials
{

namespace gui = Methane::UserInterface;

struct CubeVertex
{
    gfx::Mesh::Position position;
    gfx::Mesh::TexCoord texcoord;

    inline static const gfx::Mesh::VertexLayout layout{
        gfx::Mesh::VertexField::Position,
        gfx::Mesh::VertexField::TexCoord,
    };
};


static const gfx::Dimensions g_texture_size{ 320U, 320U };
static const float           g_scene_scale  = 22.F;
static const uint32_t        g_thread_count = std::thread::hardware_concurrency();
static const uint32_t        g_cubes_count  = static_cast<uint32_t>(std::pow(8U, 3U));

ParallelRenderingApp::ParallelRenderingApp()
    : UserInterfaceApp(
        Samples::GetGraphicsAppSettings("Methane Cube Map Array", Samples::g_default_app_options_color_with_depth_and_anim),
        { HeadsUpDisplayMode::WindowTitle },
        "Methane tutorial of cube-map array texturing")
{
    m_camera.ResetOrientation({ { 13.F, 13.F, -13.F }, { 0.F, 0.F, 0.F }, { 0.F, 1.F, 0.F } });

    // Setup animations
    GetAnimations().emplace_back(std::make_shared<Data::TimeAnimation>(std::bind(&ParallelRenderingApp::Animate, this, std::placeholders::_1, std::placeholders::_2)));
}

ParallelRenderingApp::~ParallelRenderingApp()
{
    // Wait for GPU rendering is completed to release resources
    WaitForRenderComplete();
}

void ParallelRenderingApp::Init()
{
    UserInterfaceApp::Init();

    gfx::CommandQueue& render_cmd_queue = GetRenderContext().GetRenderCommandKit().GetQueue();
    m_camera.Resize(GetRenderContext().GetSettings().frame_size);

    // Create cube mesh
    gfx::CubeMesh<CubeVertex> cube_mesh(CubeVertex::layout);

    // Create render state with program
    gfx::RenderState::Settings render_state_settings;
    render_state_settings.program_ptr = gfx::Program::Create(GetRenderContext(),
        gfx::Program::Settings
        {
            gfx::Program::Shaders
            {
                gfx::Shader::CreateVertex(GetRenderContext(), { Data::ShaderProvider::Get(), { "ParallelRendering", "CubeVS" } }),
                gfx::Shader::CreatePixel( GetRenderContext(), { Data::ShaderProvider::Get(), { "ParallelRendering", "CubePS" } }),
            },
            gfx::Program::InputBufferLayouts
            {
                gfx::Program::InputBufferLayout
                {
                    gfx::Program::InputBufferLayout::ArgumentSemantics { cube_mesh.GetVertexLayout().GetSemantics() }
                }
            },
            gfx::Program::ArgumentAccessors
            {
                { { gfx::Shader::Type::All,   "g_uniforms"      }, gfx::Program::ArgumentAccessor::Type::Mutable, true },
                { { gfx::Shader::Type::Pixel, "g_texture_array" }, gfx::Program::ArgumentAccessor::Type::Constant },
                { { gfx::Shader::Type::Pixel, "g_sampler"       }, gfx::Program::ArgumentAccessor::Type::Constant },
            },
            GetScreenRenderPattern().GetAttachmentFormats()
        }
    );
    render_state_settings.program_ptr->SetName("Render Pipeline State");
    render_state_settings.render_pattern_ptr = GetScreenRenderPatternPtr();
    render_state_settings.depth.enabled = true;
    m_render_state_ptr = gfx::RenderState::Create(GetRenderContext(), render_state_settings);

    // Create cube mesh buffer resources
    const gfx::Mesh::Subsets mesh_subsets(g_cubes_count,
                                          gfx::Mesh::Subset(gfx::Mesh::Type::Box,
                                                            gfx::Mesh::Subset::Slice(0U, cube_mesh.GetVertexCount()),
                                                            gfx::Mesh::Subset::Slice(0U, cube_mesh.GetIndexCount()),
                                                            false));
    m_cube_array_buffers_ptr = std::make_unique<MeshBuffers>(render_cmd_queue, std::move(cube_mesh), "Cube", mesh_subsets);

    // Create cube-map render target texture
    using namespace magic_enum::bitwise_operators;
    m_texture_array_ptr = gfx::Texture::CreateRenderTarget(GetRenderContext(),
            gfx::Texture::Settings::Image(g_texture_size, g_thread_count, gfx::PixelFormat::RGBA8Unorm, false,
                                          gfx::Texture::Usage::RenderTarget | gfx::Texture::Usage::ShaderRead));
    m_texture_array_ptr->SetName("Per-Thread Texture Array");

    // Create sampler for image texture
    m_texture_sampler_ptr = gfx::Sampler::Create(GetRenderContext(),
        gfx::Sampler::Settings
        {
            gfx::Sampler::Filter  { gfx::Sampler::Filter::MinMag::Linear },
            gfx::Sampler::Address { gfx::Sampler::Address::Mode::ClampToEdge }
        }
    );

    // Create frame buffer resources
    const auto uniforms_data_size = m_cube_array_buffers_ptr->GetUniformsBufferSize();
    for(ParallelRenderingFrame& frame : GetFrames())
    {
        // Create buffer for uniforms array related to all cube instances
        frame.cubes_array.uniforms_buffer_ptr = gfx::Buffer::CreateConstantBuffer(GetRenderContext(), uniforms_data_size, true, true);
        frame.cubes_array.uniforms_buffer_ptr->SetName(IndexedName("Uniforms Buffer", frame.index));

        // Configure program resource bindings
        frame.cubes_array.program_bindings_per_instance.resize(g_cubes_count);
        frame.cubes_array.program_bindings_per_instance[0] = gfx::ProgramBindings::Create(render_state_settings.program_ptr, {
            { { gfx::Shader::Type::All,   "g_uniforms"      }, { { *frame.cubes_array.uniforms_buffer_ptr, m_cube_array_buffers_ptr->GetUniformsBufferOffset(0U) } } },
            { { gfx::Shader::Type::Pixel, "g_texture_array" }, { { *m_texture_array_ptr   } } },
            { { gfx::Shader::Type::Pixel, "g_sampler"       }, { { *m_texture_sampler_ptr } } },
        }, frame.index);
        frame.cubes_array.program_bindings_per_instance[0]->SetName(fmt::format("Cube 0 Bindings {}", frame.index));

        for(uint32_t i = 1U; i < g_cubes_count; ++i)
        {
            frame.cubes_array.program_bindings_per_instance[i] = gfx::ProgramBindings::CreateCopy(*frame.cubes_array.program_bindings_per_instance[0], {
                { { gfx::Shader::Type::All, "g_uniforms" }, { { *frame.cubes_array.uniforms_buffer_ptr, m_cube_array_buffers_ptr->GetUniformsBufferOffset(i) } } }
            }, frame.index);
            frame.cubes_array.program_bindings_per_instance[i]->SetName(fmt::format("Cube {} Bindings {}", i, frame.index));
        }

        // Create parallel command list for rendering to the screen pass
        frame.parallel_render_cmd_list_ptr = gfx::ParallelRenderCommandList::Create(GetRenderContext().GetRenderCommandKit().GetQueue(), *frame.screen_pass_ptr);
        frame.parallel_render_cmd_list_ptr->SetParallelCommandListsCount(g_thread_count);
        frame.parallel_render_cmd_list_ptr->SetName(IndexedName("Cube Rendering", frame.index));
        frame.execute_cmd_list_set_ptr = gfx::CommandListSet::Create({ *frame.parallel_render_cmd_list_ptr }, frame.index);
    }
    
    // Create all resources for texture labels rendering before resources upload in UserInterfaceApp::CompleteInitialization()
    TextureLabeler::Settings texture_labeler_settings;
    texture_labeler_settings.font_size_pt = g_texture_size.GetWidth() / 4U;
    texture_labeler_settings.border_width_px = 10U;
    TextureLabeler cube_texture_labeler(GetUIContext(), GetFontProvider(), *m_texture_array_ptr, texture_labeler_settings);

    // Upload all resources, including font texture and text mesh buffers required for rendering
    UserInterfaceApp::CompleteInitialization();

    // Encode and execute texture labels rendering commands when all resources are uploaded and ready on GPU
    cube_texture_labeler.Render();

    // Initialize cube parameters
    m_cube_array_parameters = InitializeCubeArrayParameters(g_cubes_count, g_scene_scale);

    GetRenderContext().WaitForGpu(gfx::Context::WaitFor::RenderComplete);
}

ParallelRenderingApp::CubeArrayParameters ParallelRenderingApp::InitializeCubeArrayParameters(uint32_t cubes_count, float scene_scale)
{
    const size_t cbrt_count        = static_cast<size_t>(std::floor(std::cbrt(float(cubes_count))));
    const size_t cbrt_count_sqr  = cbrt_count * cbrt_count;
    const float  cbrt_count_half = static_cast<float>(cbrt_count - 1) / 2.F;

    const float ts = scene_scale / cbrt_count;
    const float median_cube_scale = ts / 2.F;
    const float cube_scale_delta = median_cube_scale / 3.F;

    std::mt19937 rng(1234U);
    std::uniform_real_distribution<float>   cube_scale_distribution(median_cube_scale - cube_scale_delta, median_cube_scale + cube_scale_delta);
    std::uniform_real_distribution<double>  rotation_speed_distribution(-0.8F, 0.8F);
    std::uniform_int_distribution<uint32_t> thread_index_distribution(0U, g_thread_count);

    CubeArrayParameters cube_array_parameters(cubes_count);

    // Position all cubes in a cube grid and assign to random threads
    for (uint32_t cube_index = 0U; cube_index < cubes_count; ++cube_index)
    {
        const float tx = static_cast<float>(cube_index % cbrt_count) - cbrt_count_half;
        const float ty = static_cast<float>(cube_index % cbrt_count_sqr / cbrt_count) - cbrt_count_half;
        const float tz = static_cast<float>(cube_index / cbrt_count_sqr) - cbrt_count_half;
        const float cs = cube_scale_distribution(rng);

        const hlslpp::float4x4 scale_matrix       = hlslpp::float4x4::scale(cs);
        const hlslpp::float4x4 translation_matrix = hlslpp::float4x4::translation(tx * ts, ty * ts, tz * ts);

        CubeParameters& cube_params = cube_array_parameters[cube_index];
        cube_params.model_matrix = hlslpp::mul(scale_matrix, translation_matrix);
        cube_params.rotation_speed_y = rotation_speed_distribution(rng);
        cube_params.rotation_speed_z = rotation_speed_distribution(rng);

        // Distribute cubes randomly between threads
        cube_params.thread_index = thread_index_distribution(rng);
    };

    // Sort cubes parameters by thread index
    // to make sure that actual cubes distrubution by render threads will match thread_index in parameters
    // NOTE-1: thread index is displayed on cube faces as text label using an element of Texture 2D Array.
    // NOTE-2: Sorting also improves rendering performance because it ensures using one texture for all cubes per thread.
    std::sort(cube_array_parameters.begin(), cube_array_parameters.end(),
              [](const CubeParameters& left, const CubeParameters& right)
              { return left.thread_index < right.thread_index; });

    // Fixup even distribution of cubes between threads
    const uint32_t cubes_count_per_thread = static_cast<uint32_t>(std::ceil(static_cast<double>(cubes_count) / g_thread_count));
    for (uint32_t cube_index = 0U; cube_index < cubes_count; ++cube_index)
    {
        cube_array_parameters[cube_index].thread_index = cube_index / cubes_count_per_thread;
    }

    return cube_array_parameters;
}

bool ParallelRenderingApp::Animate(double, double delta_seconds)
{
    m_camera.Rotate(m_camera.GetOrientation().up, static_cast<float>(delta_seconds * 360.0 / 16.0));

    const double delta_angle_rad = delta_seconds * gfx::ConstDouble::Pi;
    tf::Taskflow task_flow;
    task_flow.for_each(m_cube_array_parameters.begin(), m_cube_array_parameters.end(),
        [delta_angle_rad](CubeParameters& cube_params)
        {
            const hlslpp::float4x4 rotate_matrix = hlslpp::mul(hlslpp::float4x4::rotation_z(static_cast<float>(delta_angle_rad * cube_params.rotation_speed_z)),
                                                               hlslpp::float4x4::rotation_y(static_cast<float>(delta_angle_rad * cube_params.rotation_speed_y)));
            cube_params.model_matrix = hlslpp::mul(rotate_matrix, cube_params.model_matrix);
        });

    GetRenderContext().GetParallelExecutor().run(task_flow).get();
    return true;
}

bool ParallelRenderingApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    // Resize screen color and depth textures
    if (!UserInterfaceApp::Resize(frame_size, is_minimized))
        return false;

    m_camera.Resize(frame_size);
    return true;
}

bool ParallelRenderingApp::Update()
{
    if (!UserInterfaceApp::Update())
        return false;

    // Update MVP-matrices for all cube instances so that they are positioned in a cube grid
    tf::Taskflow task_flow;
    task_flow.for_each_index(0U, static_cast<uint32_t>(m_cube_array_parameters.size()), 1U,
        [this](const uint32_t cube_index)
        {
            const CubeParameters& cube_params = m_cube_array_parameters[cube_index];
            hlslpp::Uniforms uniforms{};
            uniforms.mvp_matrix = hlslpp::transpose(hlslpp::mul(cube_params.model_matrix, m_camera.GetViewProjMatrix()));
            uniforms.texture_index = cube_params.thread_index;
            m_cube_array_buffers_ptr->SetFinalPassUniforms(std::move(uniforms), cube_index);
        });

    GetRenderContext().GetParallelExecutor().run(task_flow).get();
    return true;
}

bool ParallelRenderingApp::Render()
{
    if (!UserInterfaceApp::Render())
        return false;

    // Update uniforms buffer related to current frame
    const ParallelRenderingFrame& frame = GetCurrentFrame();
    gfx::CommandQueue& render_cmd_queue = GetRenderContext().GetRenderCommandKit().GetQueue();
    frame.cubes_array.uniforms_buffer_ptr->SetData(m_cube_array_buffers_ptr->GetFinalPassUniformsSubresources(), render_cmd_queue);

    // Render cube instances of 'CUBE_MAP_ARRAY_SIZE' count
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Cube Rendering");
    frame.parallel_render_cmd_list_ptr->ResetWithState(*m_render_state_ptr, s_debug_group.get());
    frame.parallel_render_cmd_list_ptr->SetViewState(GetViewState());
    m_cube_array_buffers_ptr->DrawParallel(*frame.parallel_render_cmd_list_ptr, frame.cubes_array.program_bindings_per_instance);

    RenderOverlay(*frame.parallel_render_cmd_list_ptr->GetParallelCommandLists().back());

    // Commit and execute command list on render queue
    frame.parallel_render_cmd_list_ptr->Commit();
    render_cmd_queue.Execute(*frame.execute_cmd_list_set_ptr);

    // Present frame to screen
    GetRenderContext().Present();
    return true;
}

void ParallelRenderingApp::OnContextReleased(gfx::Context& context)
{
    m_cube_array_buffers_ptr.reset();
    m_texture_sampler_ptr.reset();
    m_render_state_ptr.reset();

    UserInterfaceApp::OnContextReleased(context);
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::ParallelRenderingApp().Run({ argc, argv });
}
