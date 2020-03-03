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

FILE: AsteroidsApp.cpp
Sample demonstrating parallel rendering of the distinct asteroids massive

******************************************************************************/

#include "AsteroidsApp.h"
#include "AsteroidsAppController.h"

#include <Methane/Graphics/AppCameraController.h>
#include <Methane/Data/TimeAnimation.h>
#include <Methane/Instrumentation.h>
#include <Methane/Platform/Logger.h>
#include <CLI/CLI.hpp>

#include <cassert>
#include <memory>
#include <thread>
#include <array>
#include <map>

namespace Methane::Samples
{

struct MutableParameters
{
    uint32_t instances_count;
    uint32_t unique_mesh_count;
    uint32_t textures_count;
    float    scale_ratio;
};

constexpr uint32_t g_max_complexity = 9;
static const std::array<MutableParameters, g_max_complexity+1> g_mutable_parameters = {{
    { 1000u,  35u,   10u, 0.6f  }, // 0
    { 2000u,  50u,   10u, 0.5f  }, // 1
    { 3000u,  75u,   20u, 0.45f }, // 2
    { 4000u,  100u,  20u, 0.4f  }, // 3
    { 5000u,  200u,  30u, 0.33f }, // 4
    { 10000u, 300u,  30u, 0.3f  }, // 5
    { 15000u, 400u,  40u, 0.27f }, // 6
    { 20000u, 500u,  40u, 0.23f }, // 7
    { 35000u, 750u,  50u, 0.2f  }, // 8
    { 50000u, 1000u, 50u, 0.17f }, // 9
}};

inline uint32_t GetDefaultComplexity()
{
#ifdef _DEBUG
    return 1u;
#else
    return std::thread::hardware_concurrency() / 2;
#endif
}

inline const MutableParameters& GetMutableParameters(uint32_t complexity)
{
    return g_mutable_parameters[std::min(complexity, g_max_complexity)];
}

inline const MutableParameters& GetMutableParameters()
{
    return GetMutableParameters(GetDefaultComplexity());
}

static const std::map<pal::Keyboard::State, AsteroidsAppAction> g_asteroids_action_by_keyboard_state = {
    { { pal::Keyboard::Key::F3           }, AsteroidsAppAction::ShowParameters              },
    { { pal::Keyboard::Key::RightBracket }, AsteroidsAppAction::IncreaseComplexity          },
    { { pal::Keyboard::Key::LeftBracket  }, AsteroidsAppAction::DecreaseComplexity          },
    { { pal::Keyboard::Key::P            }, AsteroidsAppAction::SwitchParallelRendering     },
    { { pal::Keyboard::Key::L            }, AsteroidsAppAction::SwitchMeshLodsColoring      },
    { { pal::Keyboard::Key::Apostrophe   }, AsteroidsAppAction::IncreaseMeshLodComplexity   },
    { { pal::Keyboard::Key::Semicolon    }, AsteroidsAppAction::DecreaseMeshLodComplexity   },
};

// Common application settings
static const std::string              g_app_help_text = "Methane sample demonstrating parallel rendering of massive randomly generated asteroids field observable with arc-ball camera.";
static const GraphicsApp::AllSettings g_app_settings  = // Application settings:
{                                                       // ====================
    {                                                   // platform_app:
        "Methane Asteroids",                            // - name
        0.8, 0.8,                                       // - width, height
        false,                                          // - is_full_screen
    },                                                  //
    {                                                   // graphics_app:
        gfx::RenderPass::Access::ShaderResources |      // - screen_pass_access
        gfx::RenderPass::Access::Samplers,              //
        true,                                           // - animations_enabled
        true,                                           // - show_hud_in_window_title
        true,                                           // - show_logo_badge
        0                                               // - default_device_index
    },                                                  //
    {                                                   // render_context:
        gfx::FrameSize(),                               // - frame_size
        gfx::PixelFormat::BGRA8Unorm,                   // - color_format
        gfx::PixelFormat::Depth32Float,                 // - depth_stencil_format
        { /* color clearing disabled */ },              // - clear_color
        gfx::DepthStencil{ 0.f, 0 },                    // - clear_depth_stencil
        3u,                                             // - frame_buffers_count
        false,                                          // - vsync_enabled
    }
};

AsteroidsApp::AsteroidsApp()
    : GraphicsApp(g_app_settings, g_app_help_text)
    , m_view_camera(m_animations, gfx::ActionCamera::Pivot::Aim)
    , m_light_camera(m_view_camera, m_animations, gfx::ActionCamera::Pivot::Aim)
    , m_scene_scale(15.f)
    , m_scene_constants(                                // Shader constants:
        {                                               // ================
            gfx::Color4f(1.f, 1.f, 1.f, 1.f),           // - light_color
            1.25f,                                      // - light_power
            0.1f,                                       // - light_ambient_factor
            4.f                                         // - light_specular_factor
        })
    , m_asteroids_array_settings(                       // Asteroids array settings:
        {                                               // ================
            m_view_camera,                              // - view_camera
            m_scene_scale,                              // - scale
            GetMutableParameters().instances_count,     // - instance_count
            GetMutableParameters().unique_mesh_count,   // - unique_mesh_count
            4u,                                         // - subdivisions_count
            GetMutableParameters().textures_count,      // - textures_count
            { 256u, 256u },                             // - texture_dimensions
            1123u,                                      // - random_seed
            13.f,                                       // - orbit_radius_ratio
            4.f,                                        // - disc_radius_ratio
            0.06f,                                      // - mesh_lod_min_screen_size
            GetMutableParameters().scale_ratio / 10.f,  // - min_asteroid_scale_ratio
            GetMutableParameters().scale_ratio,         // - max_asteroid_scale_ratio
            true,                                       // - textures_array_enabled
            true                                        // - depth_reversed
        })
    , m_asteroids_complexity(static_cast<uint32_t>(GetDefaultComplexity()))
    , m_is_parallel_rendering_enabled(true)
{
    ITT_FUNCTION_TASK();

    // NOTE: Near and Far values are swapped in camera parameters (1st value is near = max depth, 2nd value is far = min depth)
    // for Reversed-Z buffer values range [ near: 1, far 0], instead of [ near 0, far 1]
    // which is used for "from near to far" drawing order for reducing pixels overdraw
    m_view_camera.SetOrientation({ { -110.f, 75.f, 210.f }, { 0.f, -60.f, 25.f }, { 0.f, 1.f, 0.f } });
    m_view_camera.SetParameters({ 600.f /* near = max depth */, 0.01f /*far = min depth*/, 90.f /* FOV */ });
    m_view_camera.SetZoomDistanceRange({ 60.f , 400.f });

    m_light_camera.SetOrientation({ { -100.f, 120.f, 0.f }, { 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f } });
    m_light_camera.SetProjection(gfx::Camera::Projection::Orthogonal);
    m_light_camera.SetParameters({ -300.f, 300.f, 90.f });
    m_light_camera.Resize(120.f, 120.f);

    InputState().AddControllers({
        std::make_shared<AsteroidsAppController>(*this, g_asteroids_action_by_keyboard_state),
        std::make_shared<gfx::AppCameraController>(m_view_camera,  "VIEW CAMERA"),
        std::make_shared<gfx::AppCameraController>(m_light_camera, "LIGHT SOURCE",
            gfx::AppCameraController::ActionByMouseButton   { { pal::Mouse::Button::Right, gfx::ActionCamera::MouseAction::Rotate   } },
            gfx::AppCameraController::ActionByKeyboardState { { { pal::Keyboard::Key::LeftControl, pal::Keyboard::Key::L }, gfx::ActionCamera::KeyboardAction::Reset } },
            gfx::AppCameraController::ActionByKeyboardKey   { }),
    });

    const std::string options_group = "Asteroids Options";
    add_option_group(options_group);
    add_option("-c,--complexity",
               [this](CLI::results_t res) {
                       uint32_t complexity = 0;
                       if (CLI::detail::lexical_cast(res[0], complexity))
                       {
                           SetAsteroidsComplexity(complexity);
                           return true;
                       }
                       return false;
                   }, "simulation complexity", true)
        ->default_val(m_asteroids_complexity)
        ->expected(0, static_cast<int>(g_max_complexity))
        ->group(options_group);
    add_option("-s,--subdiv-count", m_asteroids_array_settings.subdivisions_count, "mesh subdivisions count", true)->group(options_group);
    add_option("-t,--texture-array", m_asteroids_array_settings.textures_array_enabled, "texture array enabled", true)->group(options_group);
    add_option("-r,--parallel-render", m_is_parallel_rendering_enabled, "parallel rendering enabled", true)->group(options_group);
}

AsteroidsApp::~AsteroidsApp()
{
    ITT_FUNCTION_TASK();
    // Wait for GPU rendering is completed to release resources
    m_sp_context->WaitForGpu(gfx::RenderContext::WaitFor::RenderComplete);
}

void AsteroidsApp::Init()
{
    ITT_FUNCTION_TASK();
    SCOPE_TIMER("AsteroidsApp::Init");

    GraphicsApp::Init();

    assert(m_sp_context);
    gfx::RenderContext& context = *m_sp_context;
    const gfx::RenderContext::Settings& context_settings = context.GetSettings();
    m_view_camera.Resize(static_cast<float>(context_settings.frame_size.width),
                         static_cast<float>(context_settings.frame_size.height));

    // Create sky-box
    m_sp_sky_box = std::make_shared<gfx::SkyBox>(context, m_image_loader, gfx::SkyBox::Settings{
        m_view_camera,
        {
            "Textures/SkyBox/Galaxy/PositiveX.jpg",
            "Textures/SkyBox/Galaxy/NegativeX.jpg",
            "Textures/SkyBox/Galaxy/PositiveY.jpg",
            "Textures/SkyBox/Galaxy/NegativeY.jpg",
            "Textures/SkyBox/Galaxy/PositiveZ.jpg",
            "Textures/SkyBox/Galaxy/NegativeZ.jpg"
        },
        m_scene_scale * 100.f,
        true, true // depth enabled and reversed
    });

    // Create planet
    m_sp_planet = std::make_shared<Planet>(context, m_image_loader, Planet::Settings{
        m_view_camera,
        m_light_camera,
        "Textures/Planet/Mars.jpg",     // texture_path
        gfx::Vector3f(0.f, 0.f, 0.f),   // position
        m_scene_scale * 3.f,            // scale
        0.1f,                           // spin_velocity_rps
        true,                           // depth_reversed
        true,                           // mip-mapped
        -1.f,                           // lod_bias
    });

    // Create asteroids array
    m_sp_asteroids_array = m_sp_asteroids_array_state
                         ? std::make_unique<AsteroidsArray>(context, m_asteroids_array_settings, *m_sp_asteroids_array_state)
                         : std::make_unique<AsteroidsArray>(context, m_asteroids_array_settings);

    const Data::Size constants_data_size         = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(Constants)));
    const Data::Size scene_uniforms_data_size    = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(SceneUniforms)));
    const Data::Size asteroid_uniforms_data_size = m_sp_asteroids_array->GetUniformsBufferSize();

    // Create constants buffer for frame rendering
    m_sp_const_buffer = gfx::Buffer::CreateConstantBuffer(context, constants_data_size);
    m_sp_const_buffer->SetName("Constants Buffer");
    m_sp_const_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&m_scene_constants), sizeof(m_scene_constants) } });

    // ========= Per-Frame Data =========
    for(AsteroidsFrame& frame : m_frames)
    {
        // Create initial screen pass for asteroids rendering
        gfx::RenderPass::Settings initial_screen_pass_settings = frame.sp_screen_pass->GetSettings();
        initial_screen_pass_settings.depth_attachment.store_action = gfx::RenderPass::Attachment::StoreAction::Store;
        frame.sp_initial_screen_pass = gfx::RenderPass::Create(context, initial_screen_pass_settings);

        // Create final screen pass for sky-box and planet rendering
        gfx::RenderPass::Settings final_screen_pass_settings = frame.sp_screen_pass->GetSettings();
        final_screen_pass_settings.color_attachments[0].load_action = gfx::RenderPass::Attachment::LoadAction::Load;
        final_screen_pass_settings.depth_attachment.load_action     = gfx::RenderPass::Attachment::LoadAction::Load;
        frame.sp_final_screen_pass = gfx::RenderPass::Create(context, final_screen_pass_settings);

        // Create parallel command list for asteroids rendering
        frame.sp_parallel_cmd_list = gfx::ParallelRenderCommandList::Create(context.GetRenderCommandQueue(), *frame.sp_initial_screen_pass);
        frame.sp_parallel_cmd_list->SetParallelCommandListsCount(std::thread::hardware_concurrency());
        frame.sp_parallel_cmd_list->SetName(IndexedName("Parallel Rendering", frame.index));

        // Create serial command list for asteroids rendering
        frame.sp_serial_cmd_list = gfx::RenderCommandList::Create(context.GetRenderCommandQueue(), *frame.sp_initial_screen_pass);
        frame.sp_serial_cmd_list->SetName(IndexedName("Serial Rendering", frame.index));

        // Create final command list for sky-box and planet rendering
        frame.sp_final_cmd_list = gfx::RenderCommandList::Create(context.GetRenderCommandQueue(), *frame.sp_final_screen_pass);
        frame.sp_final_cmd_list->SetName(IndexedName("Final Rendering", frame.index));

        // Create uniforms buffer with volatile parameters for the whole scene rendering
        frame.sp_scene_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(context, scene_uniforms_data_size);
        frame.sp_scene_uniforms_buffer->SetName(IndexedName("Scene Uniforms Buffer", frame.index));

        // Create uniforms buffer for Sky-Box rendering
        frame.skybox.sp_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(context, sizeof(gfx::SkyBox::Uniforms));
        frame.skybox.sp_uniforms_buffer->SetName(IndexedName("Sky-box Uniforms Buffer", frame.index));

        // Create uniforms buffer for Planet rendering
        frame.planet.sp_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(context, sizeof(Planet::Uniforms));
        frame.planet.sp_uniforms_buffer->SetName(IndexedName("Planet Uniforms Buffer", frame.index));

        // Create uniforms buffer for Asteroids array rendering
        frame.asteroids.sp_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(context, asteroid_uniforms_data_size, true);
        frame.asteroids.sp_uniforms_buffer->SetName(IndexedName("Asteroids Array Uniforms Buffer", frame.index));

        // Resource bindings for Sky-Box rendering
        frame.skybox.program_bindings_per_instance.resize(1);
        frame.skybox.program_bindings_per_instance[0] = m_sp_sky_box->CreateProgramBindings(frame.skybox.sp_uniforms_buffer);

        // Resource bindings for Planet rendering
        frame.planet.program_bindings_per_instance.resize(1);
        frame.planet.program_bindings_per_instance[0] = m_sp_planet->CreateProgramBindings(m_sp_const_buffer, frame.planet.sp_uniforms_buffer);

        // Resource bindings for Asteroids rendering
        frame.asteroids.program_bindings_per_instance = m_sp_asteroids_array->CreateProgramBindings(m_sp_const_buffer, frame.sp_scene_uniforms_buffer, frame.asteroids.sp_uniforms_buffer);
    }

    // Setup animations
    m_animations.push_back(std::make_shared<Data::TimeAnimation>(std::bind(&Planet::Update, m_sp_planet.get(), std::placeholders::_1, std::placeholders::_2)));
    m_animations.push_back(std::make_shared<Data::TimeAnimation>(std::bind(&AsteroidsArray::Update, m_sp_asteroids_array.get(), std::placeholders::_1, std::placeholders::_2)));

    // Complete initialization of render context:
    //  - allocate deferred descriptor heaps with calculated sizes
    //  - execute commands to upload resources to GPU
    context.CompleteInitialization();

    pal::PrintToDebugOutput(GetParametersString());
}

bool AsteroidsApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    ITT_FUNCTION_TASK();

    // Resize screen color and depth textures
    if (!GraphicsApp::Resize(frame_size, is_minimized))
        return false;
    
    // Update frame buffer and depth textures in initial & final render passes
    for (AsteroidsFrame& frame : m_frames)
    {
        assert(!!frame.sp_initial_screen_pass);
        gfx::RenderPass::Settings initial_pass_settings = frame.sp_initial_screen_pass->GetSettings();
        initial_pass_settings.color_attachments[0].wp_texture = frame.sp_screen_texture;
        initial_pass_settings.depth_attachment.wp_texture = m_sp_depth_texture;
        frame.sp_initial_screen_pass->Update(initial_pass_settings);
        
        assert(!!frame.sp_final_screen_pass);
        gfx::RenderPass::Settings final_pass_settings = frame.sp_final_screen_pass->GetSettings();
        final_pass_settings.color_attachments[0].wp_texture = frame.sp_screen_texture;
        final_pass_settings.depth_attachment.wp_texture = m_sp_depth_texture;
        frame.sp_final_screen_pass->Update(final_pass_settings);
    }

    m_sp_sky_box->Resize(frame_size);
    m_sp_planet->Resize(frame_size);
    m_sp_asteroids_array->Resize(frame_size);
    m_view_camera.Resize(static_cast<float>(frame_size.width), static_cast<float>(frame_size.height));

    return true;
}

bool AsteroidsApp::Update()
{
    ITT_FUNCTION_TASK();
    SCOPE_TIMER("AsteroidsApp::Update");

    if (!GraphicsApp::Update())
        return false;

    // Update scene uniforms
    m_scene_uniforms.eye_position    = gfx::Vector4f(m_view_camera.GetOrientation().eye, 1.f);
    m_scene_uniforms.light_position  = m_light_camera.GetOrientation().eye;

    m_sp_sky_box->Update();
    return true;
}

bool AsteroidsApp::Render()
{
    ITT_FUNCTION_TASK();
    SCOPE_TIMER("AsteroidsApp::Render");

    // Render only when context is ready
    assert(!!m_sp_context);
    if (!m_sp_context->ReadyToRender() || !GraphicsApp::Render())
        return false;

    // Wait for previous frame rendering is completed and switch to next frame
    m_sp_context->WaitForGpu(gfx::RenderContext::WaitFor::FramePresented);
    AsteroidsFrame& frame = GetCurrentFrame();

    // Upload uniform buffers to GPU
    frame.sp_scene_uniforms_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&m_scene_uniforms), sizeof(SceneUniforms) } });

    // Asteroids rendering in parallel or in main thread
    Refs<gfx::CommandList> execute_cmd_lists;
    if (m_is_parallel_rendering_enabled)
    {
        GetAsteroidsArray().DrawParallel(*frame.sp_parallel_cmd_list, frame.asteroids);
        frame.sp_parallel_cmd_list->Commit();
        execute_cmd_lists.push_back(*frame.sp_parallel_cmd_list);
    }
    else
    {
        GetAsteroidsArray().Draw(*frame.sp_serial_cmd_list, frame.asteroids);
        frame.sp_serial_cmd_list->Commit();
        execute_cmd_lists.push_back(*frame.sp_serial_cmd_list);
    }
    
    // Draw planet and sky-box after asteroids to minimize pixel overdraw
    m_sp_planet->Draw(*frame.sp_final_cmd_list, frame.planet);
    m_sp_sky_box->Draw(*frame.sp_final_cmd_list, frame.skybox);
    RenderOverlay(*frame.sp_final_cmd_list);
    frame.sp_final_cmd_list->Commit();
    execute_cmd_lists.push_back(*frame.sp_final_cmd_list);

    // Execute rendering commands and present frame to screen
    m_sp_context->GetRenderCommandQueue().Execute(execute_cmd_lists);
    m_sp_context->Present();

    return true;
}

void AsteroidsApp::OnContextReleased()
{
    ITT_FUNCTION_TASK();
    FLUSH_SCOPE_TIMINGS();

    if (m_sp_asteroids_array)
    {
        m_sp_asteroids_array_state = m_sp_asteroids_array->GetState();
    }

    m_animations.clear();
    m_sp_sky_box.reset();
    m_sp_planet.reset();
    m_sp_asteroids_array.reset();
    m_sp_const_buffer.reset();

    GraphicsApp::OnContextReleased();
}

void AsteroidsApp::SetAsteroidsComplexity(uint32_t asteroids_complexity)
{
    ITT_FUNCTION_TASK();

    asteroids_complexity = std::min(g_max_complexity, asteroids_complexity);
    if (m_asteroids_complexity == asteroids_complexity)
        return;

    if (m_sp_context)
        m_sp_context->WaitForGpu(gfx::RenderContext::WaitFor::RenderComplete);

    m_asteroids_complexity = asteroids_complexity;

    const MutableParameters& mutable_parameters         = GetMutableParameters(m_asteroids_complexity);
    m_asteroids_array_settings.instance_count           = mutable_parameters.instances_count;
    m_asteroids_array_settings.unique_mesh_count        = mutable_parameters.unique_mesh_count;
    m_asteroids_array_settings.textures_count           = mutable_parameters.textures_count;
    m_asteroids_array_settings.min_asteroid_scale_ratio = mutable_parameters.scale_ratio / 10.f;
    m_asteroids_array_settings.max_asteroid_scale_ratio = mutable_parameters.scale_ratio;

    m_sp_asteroids_array.reset();
    m_sp_asteroids_array_state.reset();

    if (m_sp_context)
        m_sp_context->Reset();
}

void AsteroidsApp::SetParallelRenderingEnabled(bool is_parallel_rendering_enabled)
{
    ITT_FUNCTION_TASK();
    if (m_is_parallel_rendering_enabled == is_parallel_rendering_enabled)
        return;

    FLUSH_SCOPE_TIMINGS();
    m_is_parallel_rendering_enabled = is_parallel_rendering_enabled;
    pal::PrintToDebugOutput(GetParametersString());
}


AsteroidsArray& AsteroidsApp::GetAsteroidsArray() const
{
    ITT_FUNCTION_TASK();
    assert(!!m_sp_asteroids_array);
    return *m_sp_asteroids_array;
}

std::string AsteroidsApp::GetParametersString() const
{
    ITT_FUNCTION_TASK();

    std::stringstream ss;
    ss << std::endl << "Asteroids simulation parameters:"
       << std::endl << "  - simulation complexity [0.."  << g_max_complexity << "]: " << m_asteroids_complexity
       << std::endl << "  - asteroid instances count: "  << m_asteroids_array_settings.instance_count
       << std::endl << "  - unique meshes count: "       << m_asteroids_array_settings.unique_mesh_count
       << std::endl << "  - mesh subdivisions count: "   << m_asteroids_array_settings.subdivisions_count
       << std::endl << "  - unique textures count: "     << m_asteroids_array_settings.textures_count << " "
                                                         << static_cast<std::string>(m_asteroids_array_settings.texture_dimensions)
       << std::endl << "  - textures array binding: "    << (m_asteroids_array_settings.textures_array_enabled ? "enabled" : "disabled")
       << std::endl << "  - parallel rendering: "        << (m_is_parallel_rendering_enabled ? "enabled" : "disabled")
       << std::endl << "  - CPU hardware thread count: " << std::thread::hardware_concurrency();

    return ss.str();
}

} // namespace Methane::Samples

int main(int argc, const char* argv[])
{
    SCOPE_TIMER_INITIALIZE(Methane::Platform::Logger);
    return Methane::Samples::AsteroidsApp().Run({ argc, argv });
}
