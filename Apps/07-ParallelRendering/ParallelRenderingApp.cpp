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
#include "ParallelRenderingAppController.h"

#include <Methane/Tutorials/TextureLabeler.h>
#include <Methane/Tutorials/AppSettings.h>
#include <Methane/Graphics/CubeMesh.hpp>
#include <Methane/Data/TimeAnimation.h>
#include <Methane/Instrumentation.h>

#include <taskflow/algorithm/for_each.hpp>
#include <taskflow/algorithm/sort.hpp>
#include <cmath>
#include <random>
#include <algorithm>

namespace Methane::Tutorials
{

#define EXPLICIT_PARALLEL_RENDERING_ENABLED

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

namespace pin = Methane::Platform::Input;
static const std::map<pin::Keyboard::State, ParallelRenderingAppAction> g_parallel_rendering_action_by_keyboard_state{
    { { pin::Keyboard::Key::P            }, ParallelRenderingAppAction::SwitchParallelRendering },
    { { pin::Keyboard::Key::Equal        }, ParallelRenderingAppAction::IncreaseCubesGridSize },
    { { pin::Keyboard::Key::Minus        }, ParallelRenderingAppAction::DecreaseCubesGridSize },
    { { pin::Keyboard::Key::RightBracket }, ParallelRenderingAppAction::IncreaseRenderThreadsCount },
    { { pin::Keyboard::Key::LeftBracket  }, ParallelRenderingAppAction::DecreaseRenderThreadsCount },
};

bool ParallelRenderingApp::Settings::operator==(const Settings& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(cubes_grid_size, render_thread_count, parallel_rendering_enabled) ==
           std::tie(other.cubes_grid_size, other.render_thread_count, other.parallel_rendering_enabled);
}

uint32_t ParallelRenderingApp::Settings::GetTotalCubesCount() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<uint32_t>(std::pow(cubes_grid_size, 3U));
}

uint32_t ParallelRenderingApp::Settings::GetActiveRenderThreadCount() const noexcept
{
    META_FUNCTION_TASK();
    return parallel_rendering_enabled ? render_thread_count : 1U;
}

ParallelRenderingApp::ParallelRenderingApp()
    : UserInterfaceApp(
        GetGraphicsTutorialAppSettings("Methane Parallel Rendering", AppOptions::GetDefaultWithColorDepthAndAnim()),
        GetUserInterfaceTutorialAppSettings(AppOptions::GetDefaultWithColorDepthAndAnim()),
        "Methane tutorial of parallel rendering")
{
    META_FUNCTION_TASK();
    m_camera.ResetOrientation({ { 13.F, 13.F, -13.F }, { 0.F, 0.F, 0.F }, { 0.F, 1.F, 0.F } });

    AddInputControllers({
        std::make_shared<ParallelRenderingAppController>(*this, g_parallel_rendering_action_by_keyboard_state)
    });

    const std::string options_group = "Parallel Rendering Options";
    add_option_group(options_group);
    add_option("-p,--parallel-render", m_settings.parallel_rendering_enabled, "enable parallel rendering")->group(options_group);
    add_option("-g,--cubes-grid-size", m_settings.cubes_grid_size,            "cubes grid size")->group(options_group);
    add_option("-t,--threads-count",   m_settings.render_thread_count,        "render threads count")->group(options_group);

    // Setup animations
    GetAnimations().emplace_back(std::make_shared<Data::TimeAnimation>(std::bind(&ParallelRenderingApp::Animate, this, std::placeholders::_1, std::placeholders::_2)));

    ShowParameters();
}

ParallelRenderingApp::~ParallelRenderingApp()
{
    META_FUNCTION_TASK();
    // Wait for GPU rendering is completed to release resources
    WaitForRenderComplete();
}

void ParallelRenderingApp::Init()
{
    META_FUNCTION_TASK();
    UserInterfaceApp::Init();

    const rhi::CommandQueue render_cmd_queue = GetRenderContext().GetRenderCommandKit().GetQueue();
    m_camera.Resize(GetRenderContext().GetSettings().frame_size);

    // Create cube mesh
    gfx::CubeMesh<CubeVertex> cube_mesh(CubeVertex::layout);

    // Create render state with program
    rhi::RenderState::Settings render_state_settings
    {
        GetRenderContext().CreateProgram(
            rhi::Program::Settings
            {
                rhi::Program::ShaderSet
                {
                    { rhi::ShaderType::Vertex, { Data::ShaderProvider::Get(), { "ParallelRendering", "CubeVS" } } },
                    { rhi::ShaderType::Pixel,  { Data::ShaderProvider::Get(), { "ParallelRendering", "CubePS" } } },
                },
                rhi::ProgramInputBufferLayouts
                {
                    rhi::Program::InputBufferLayout
                    {
                        rhi::Program::InputBufferLayout::ArgumentSemantics { cube_mesh.GetVertexLayout().GetSemantics() }
                    }
                },
                rhi::ProgramArgumentAccessors
                {
                    { { rhi::ShaderType::All,   "g_uniforms"      }, rhi::ProgramArgumentAccessor::Type::Mutable, true },
                    { { rhi::ShaderType::Pixel, "g_texture_array" }, rhi::ProgramArgumentAccessor::Type::Constant },
                    { { rhi::ShaderType::Pixel, "g_sampler"       }, rhi::ProgramArgumentAccessor::Type::Constant },
                },
                GetScreenRenderPattern().GetAttachmentFormats()
            }
        ),
        GetScreenRenderPattern()
    };
    render_state_settings.program.SetName("Render Pipeline State");
    render_state_settings.depth.enabled = true;
    m_render_state = GetRenderContext().CreateRenderState( render_state_settings);

    // Create cube mesh buffer resources
    const uint32_t cubes_count = m_settings.GetTotalCubesCount();
    const gfx::Mesh::Subsets mesh_subsets(cubes_count,
                                          gfx::Mesh::Subset(gfx::Mesh::Type::Box,
                                                            gfx::Mesh::Subset::Slice(0U, cube_mesh.GetVertexCount()),
                                                            gfx::Mesh::Subset::Slice(0U, cube_mesh.GetIndexCount()),
                                                            false));
    m_cube_array_buffers_ptr = std::make_unique<MeshBuffers>(render_cmd_queue, std::move(cube_mesh), "Cube", mesh_subsets);

    // Create cube-map render target texture
    m_texture_array = GetRenderContext().CreateTexture(
                         rhi::Texture::Settings::ForImage(g_texture_size, m_settings.render_thread_count, gfx::PixelFormat::RGBA8Unorm, false,
                                                          rhi::ResourceUsageMask({ rhi::ResourceUsage::RenderTarget, rhi::ResourceUsage::ShaderRead })));
    m_texture_array.SetName("Per-Thread Texture Array");

    // Create sampler for image texture
    m_texture_sampler = GetRenderContext().CreateSampler(
        rhi::Sampler::Settings
        {
            rhi::Sampler::Filter  { rhi::Sampler::Filter::MinMag::Linear },
            rhi::Sampler::Address { rhi::Sampler::Address::Mode::ClampToEdge }
        }
    );

    // Create frame buffer resources
    const Data::Size uniforms_data_size = m_cube_array_buffers_ptr->GetUniformsBufferSize();
    const Data::Size uniform_data_size = MeshBuffers::GetUniformSize();
    tf::Taskflow program_bindings_task_flow;
    for(ParallelRenderingFrame& frame : GetFrames())
    {
        // Create buffer for uniforms array related to all cube instances
        frame.cubes_array.uniforms_buffer = GetRenderContext().CreateBuffer(rhi::BufferSettings::ForConstantBuffer(uniforms_data_size, true, true));
        frame.cubes_array.uniforms_buffer.SetName(fmt::format("Uniforms Buffer {}", frame.index));

        // Configure program resource bindings
        frame.cubes_array.program_bindings_per_instance.resize(cubes_count);
        frame.cubes_array.program_bindings_per_instance[0] = render_state_settings.program.CreateBindings({
            { { rhi::ShaderType::All,   "g_uniforms"      }, { { frame.cubes_array.uniforms_buffer.GetInterface(), m_cube_array_buffers_ptr->GetUniformsBufferOffset(0U), uniform_data_size } } },
            { { rhi::ShaderType::Pixel, "g_texture_array" }, { { m_texture_array.GetInterface()   } } },
            { { rhi::ShaderType::Pixel, "g_sampler"       }, { { m_texture_sampler.GetInterface() } } },
        }, frame.index);
        frame.cubes_array.program_bindings_per_instance[0].SetName(fmt::format("Cube 0 Bindings {}", frame.index));

        program_bindings_task_flow.for_each_index(1U, cubes_count, 1U,
            [this, &frame, uniform_data_size](const uint32_t cube_index)
            {
                META_UNUSED(uniform_data_size); // workaround for Clang error unused-lambda-capture uniform_data_size (false positive)
                rhi::ProgramBindings& cube_program_bindings = frame.cubes_array.program_bindings_per_instance[cube_index];
                cube_program_bindings = rhi::ProgramBindings(frame.cubes_array.program_bindings_per_instance[0], {
                    {
                        { rhi::ShaderType::All, "g_uniforms" },
                        { { frame.cubes_array.uniforms_buffer.GetInterface(), m_cube_array_buffers_ptr->GetUniformsBufferOffset(cube_index), uniform_data_size } }
                    }
                }, frame.index);
                cube_program_bindings.SetName(fmt::format("Cube {} Bindings {}", cube_index, frame.index));
            });

        if (m_settings.parallel_rendering_enabled)
        {
            // Create parallel command list for rendering to the screen pass
            frame.parallel_render_cmd_list = render_cmd_queue.CreateParallelRenderCommandList(frame.screen_pass);
            frame.parallel_render_cmd_list.SetParallelCommandListsCount(m_settings.GetActiveRenderThreadCount());
            frame.parallel_render_cmd_list.SetValidationEnabled(false);
            frame.parallel_render_cmd_list.SetName(fmt::format("Parallel Cubes Rendering {}", frame.index));
            frame.execute_cmd_list_set = rhi::CommandListSet({ frame.parallel_render_cmd_list.GetInterface() }, frame.index);
        }
        else
        {
            // Create serial command list for rendering to the screen pass
            frame.serial_render_cmd_list = render_cmd_queue.CreateRenderCommandList(frame.screen_pass);
            frame.serial_render_cmd_list.SetName(fmt::format("Serial Cubes Rendering {}", frame.index));
            frame.serial_render_cmd_list.SetValidationEnabled(false);
            frame.execute_cmd_list_set = rhi::CommandListSet({ frame.serial_render_cmd_list.GetInterface() }, frame.index);
        }
    }
    
    // Execute parallel program bindings copy initialization for all cubes
    GetRenderContext().GetParallelExecutor().run(program_bindings_task_flow).get();
    
    // Create all resources for texture labels rendering before resources upload in UserInterfaceApp::CompleteInitialization()
    TextureLabeler::Settings texture_labeler_settings;
    texture_labeler_settings.font_size_pt = g_texture_size.GetWidth() / 4U;
    texture_labeler_settings.border_width_px = 10U;
    TextureLabeler cube_texture_labeler(GetUIContext(), GetFontContext(), m_texture_array,
                                        rhi::ResourceState::ShaderResource, texture_labeler_settings);

    // Upload all resources, including font texture and text mesh buffers required for rendering
    UserInterfaceApp::CompleteInitialization();

    // Encode and execute texture labels rendering commands when all resources are uploaded and ready on GPU
    cube_texture_labeler.Render();

    // Initialize cube parameters
    m_cube_array_parameters = InitializeCubeArrayParameters();

    // Update initial resource states before asteroids drawing without applying barriers on GPU to let automatic state propagation from Common state work
    m_cube_array_buffers_ptr->CreateBeginningResourceBarriers().ApplyTransitions();

    GetRenderContext().WaitForGpu(rhi::IContext::WaitFor::RenderComplete);
}

ParallelRenderingApp::CubeArrayParameters ParallelRenderingApp::InitializeCubeArrayParameters() const
{
    META_FUNCTION_TASK();
    const uint32_t cubes_count     = m_settings.GetTotalCubesCount();
    const auto     cbrt_count      = static_cast<size_t>(std::floor(std::cbrt(static_cast<float>(cubes_count))));
    const size_t   cbrt_count_sqr  = cbrt_count * cbrt_count;
    const float    cbrt_count_half = static_cast<float>(cbrt_count - 1) / 2.F;

    const float ts = g_scene_scale / static_cast<float>(cbrt_count);
    const float median_cube_scale = ts / 2.F;
    const float cube_scale_delta = median_cube_scale / 3.F;

    std::mt19937 rng(1234U); // NOSONAR - using pseudorandom generator is safe here
    std::uniform_real_distribution<float>   cube_scale_distribution(median_cube_scale - cube_scale_delta, median_cube_scale + cube_scale_delta);
    std::uniform_real_distribution<double>  rotation_speed_distribution(-0.8F, 0.8F);
    std::uniform_int_distribution<uint32_t> thread_index_distribution(0U, m_settings.render_thread_count);

    CubeArrayParameters cube_array_parameters(cubes_count);

    // Position all cubes in a cube grid and assign to random threads
    tf::Taskflow task_flow;
    tf::Task init_task = task_flow.for_each_index(0U, cubes_count, 1U,
        [&rng, &cube_array_parameters, &cube_scale_distribution, &rotation_speed_distribution, &thread_index_distribution,
         ts, cbrt_count, cbrt_count_sqr, cbrt_count_half](const uint32_t cube_index)
        {
            const float tx = static_cast<float>(cube_index % cbrt_count) - cbrt_count_half;
            const float ty = static_cast<float>(cube_index % cbrt_count_sqr / cbrt_count) - cbrt_count_half;
            const float tz = static_cast<float>(cube_index / cbrt_count_sqr) - cbrt_count_half;
            const float cs = cube_scale_distribution(rng);

            const hlslpp::float4x4 scale_matrix = hlslpp::float4x4::scale(cs);
            const hlslpp::float4x4 translation_matrix = hlslpp::float4x4::translation(tx * ts, ty * ts, tz * ts);

            CubeParameters& cube_params = cube_array_parameters[cube_index];
            cube_params.model_matrix = hlslpp::mul(scale_matrix, translation_matrix);
            cube_params.rotation_speed_y = rotation_speed_distribution(rng);
            cube_params.rotation_speed_z = rotation_speed_distribution(rng);

            // Distribute cubes randomly between threads
            cube_params.thread_index = thread_index_distribution(rng);
        });

    // Sort cubes parameters by thread index
    // to make sure that actual cubes distribution by render threads will match thread_index in parameters
    // NOTE-1: thread index is displayed on cube faces as text label using an element of Texture 2D Array.
    // NOTE-2: Sorting also improves rendering performance because it ensures using one texture for all cubes per thread.
    tf::Task sort_task = task_flow.sort(cube_array_parameters.begin(), cube_array_parameters.end(),
                   [](const CubeParameters& left, const CubeParameters& right)
                   { return left.thread_index < right.thread_index; });

    // Fixup even distribution of cubes between threads
    const auto cubes_count_per_thread = static_cast<uint32_t>(std::ceil(static_cast<double>(cubes_count) / m_settings.render_thread_count));
    tf::Task even_task = task_flow.for_each_index(0U, cubes_count, 1U,
        [&cube_array_parameters, cubes_count_per_thread](const uint32_t cube_index)
        {
            cube_array_parameters[cube_index].thread_index = cube_index / cubes_count_per_thread;
        });

    init_task.precede(sort_task);
    sort_task.precede(even_task);

    // Execute parallel initialization of cube array parameters
    GetRenderContext().GetParallelExecutor().run(task_flow).get();
    return cube_array_parameters;
}

bool ParallelRenderingApp::Animate(double, double delta_seconds)
{
    META_FUNCTION_TASK();
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
    META_FUNCTION_TASK();
    // Resize screen color and depth textures
    if (!UserInterfaceApp::Resize(frame_size, is_minimized))
        return false;

    m_camera.Resize(frame_size);
    return true;
}

bool ParallelRenderingApp::Update()
{
    META_FUNCTION_TASK();
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
    META_FUNCTION_TASK();
    if (!UserInterfaceApp::Render())
        return false;

    // Update uniforms buffer related to current frame
    const ParallelRenderingFrame& frame  = GetCurrentFrame();
    const rhi::CommandQueue render_cmd_queue = GetRenderContext().GetRenderCommandKit().GetQueue();
    frame.cubes_array.uniforms_buffer.SetData(render_cmd_queue, m_cube_array_buffers_ptr->GetFinalPassUniformsSubresource());

    // Render cube instances of 'CUBE_MAP_ARRAY_SIZE' count
    if (m_settings.parallel_rendering_enabled)
    {
        META_DEBUG_GROUP_VAR(s_debug_group, "Parallel Cubes Rendering");
        frame.parallel_render_cmd_list.ResetWithState(m_render_state, &s_debug_group);
        frame.parallel_render_cmd_list.SetViewState(GetViewState());

#ifdef EXPLICIT_PARALLEL_RENDERING_ENABLED
        const std::vector<rhi::RenderCommandList>& render_cmd_lists = frame.parallel_render_cmd_list.GetParallelCommandLists();
        const uint32_t instance_count_per_command_list = Data::DivCeil(m_cube_array_buffers_ptr->GetInstanceCount(), static_cast<uint32_t>(render_cmd_lists.size()));

        // Generate thread tasks for each of parallel render command lists to encode cubes rendering commands
        tf::Taskflow render_task_flow;
        render_task_flow.for_each_index(0U, static_cast<uint32_t>(render_cmd_lists.size()), 1U,
            [this, &frame, &render_cmd_lists, instance_count_per_command_list](const uint32_t cmd_list_index)
            {
                const uint32_t begin_instance_index = cmd_list_index * instance_count_per_command_list;
                const uint32_t end_instance_index = std::min(begin_instance_index + instance_count_per_command_list, m_cube_array_buffers_ptr->GetInstanceCount());
                RenderCubesRange(render_cmd_lists[cmd_list_index], frame.cubes_array.program_bindings_per_instance, begin_instance_index, end_instance_index);
            }
        );

        // Execute rendering in multiple threads
        GetRenderContext().GetParallelExecutor().run(render_task_flow).get();
#else
        // The same parallel rendering is done inside of MeshBuffers::DrawParallel helper function
        m_cube_array_buffers_ptr->DrawParallel(*frame.parallel_render_cmd_list_ptr, frame.cubes_array.program_bindings_per_instance);
#endif

        RenderOverlay(frame.parallel_render_cmd_list.GetParallelCommandLists().back());
        frame.parallel_render_cmd_list.Commit();
    }
    else
    {
        META_DEBUG_GROUP_VAR(s_debug_group, "Serial Cubes Rendering");
        frame.serial_render_cmd_list.ResetWithState(m_render_state, &s_debug_group);
        frame.serial_render_cmd_list.SetViewState(GetViewState());

#ifdef EXPLICIT_PARALLEL_RENDERING_ENABLED
        RenderCubesRange(frame.serial_render_cmd_list, frame.cubes_array.program_bindings_per_instance, 0U, m_cube_array_buffers_ptr->GetInstanceCount());
#else
        m_cube_array_buffers_ptr->Draw(frame.serial_render_cmd_list, frame.cubes_array.program_bindings_per_instance);
#endif

        RenderOverlay(frame.serial_render_cmd_list);
        frame.serial_render_cmd_list.Commit();
    }

    // Execute command lists on render queue and present frame to screen
    render_cmd_queue.Execute(frame.execute_cmd_list_set);
    GetRenderContext().Present();
    return true;
}

void ParallelRenderingApp::RenderCubesRange(const rhi::RenderCommandList& render_cmd_list,
                                            const std::vector<rhi::ProgramBindings>& program_bindings_per_instance,
                                            uint32_t begin_instance_index, const uint32_t end_instance_index) const
{
    META_FUNCTION_TASK();
    // Resource barriers are not set for vertex and index buffers, since it works with automatic state propagation from Common state
    render_cmd_list.SetVertexBuffers(m_cube_array_buffers_ptr->GetVertexBuffers(), false);
    render_cmd_list.SetIndexBuffer(m_cube_array_buffers_ptr->GetIndexBuffer(), false);

    for (uint32_t instance_index = begin_instance_index; instance_index < end_instance_index; ++instance_index)
    {
        // Constant argument bindings are applied once per command list, mutables are applied always
        // Bound resources are retained by command list during its lifetime, but only for the first binding instance (since all binding instances use the same resource objects)
        rhi::ProgramBindingsApplyBehaviorMask bindings_apply_behavior;
        bindings_apply_behavior.SetBitOn(rhi::ProgramBindingsApplyBehavior::ConstantOnce);
        if (instance_index == begin_instance_index)
            bindings_apply_behavior.SetBitOn(rhi::ProgramBindingsApplyBehavior::RetainResources);

        render_cmd_list.SetProgramBindings(program_bindings_per_instance[instance_index], bindings_apply_behavior);
        render_cmd_list.DrawIndexed(rhi::RenderPrimitive::Triangle);
    }
}

std::string ParallelRenderingApp::GetParametersString()
{
    META_FUNCTION_TASK();
    std::stringstream ss;

    ss << "Parallel Rendering parameters:"
        << std::endl << "  - parallel rendering:   " << (m_settings.parallel_rendering_enabled ? "ON" : "OFF")
        << std::endl << "  - render threads count: " << m_settings.GetActiveRenderThreadCount()
        << std::endl << "  - cubes grid size:      " << m_settings.cubes_grid_size
        << std::endl << "  - total cubes count:    " << m_settings.GetTotalCubesCount()
        << std::endl << "  - texture array size:   " << g_texture_size.GetWidth() <<
                                               " x " << g_texture_size.GetHeight() <<
                                                " [" << m_settings.render_thread_count << "]";

    return ss.str();
}

void ParallelRenderingApp::SetSettings(const Settings& settings)
{
    META_FUNCTION_TASK();
    if (m_settings == settings)
        return;

    m_settings = settings;
    GetRenderContext().Reset();
}

void ParallelRenderingApp::OnContextReleased(rhi::IContext& context)
{
    META_FUNCTION_TASK();
    m_cube_array_buffers_ptr.reset();
    m_texture_array = {};
    m_texture_sampler = {};
    m_render_state = {};

    UserInterfaceApp::OnContextReleased(context);
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::ParallelRenderingApp().Run({ argc, argv });
}
