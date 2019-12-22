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

FILE: AsteroidsApp.cpp
Sample demonstrating parallel rendering of the distinct asteroids massive

******************************************************************************/

#include "AsteroidsApp.h"

#include <Methane/Graphics/AppCameraController.h>
#include <Methane/Data/TimeAnimation.h>
#include <Methane/Data/Instrumentation.h>

#include <cassert>
#include <memory>
#include <array>
#include <thread>

#define PARALLEL_RENDERING_ENABLED 1

namespace Methane::Samples
{

template<typename ValueT, size_t N>
using ParamValueByCoresCount = std::array<ValueT, N>;

template<typename ValueT, size_t N>
inline ValueT GetParamValueByCoresCount(const ParamValueByCoresCount<ValueT, N>& param_values_by_cores_count)
{
    const size_t cores_count = std::thread::hardware_concurrency() / 2;
    return param_values_by_cores_count[std::min(cores_count, N - 1)];
}

constexpr size_t g_max_cores_count = 11;
static const ParamValueByCoresCount<uint32_t, g_max_cores_count> g_instaces_count = {
//  [0]   [1]    [2]    [3]    [4]    [5]    [6]     [7]     [8]     [9]     [10]
    500u, 1000u, 2000u, 3000u, 4000u, 5000u, 10000u, 15000u, 20000u, 35000u, 50000u,
};
static const ParamValueByCoresCount<uint32_t, g_max_cores_count> g_mesh_count = {
    50u,  70u,   100u,  150u,  200u,  400u,  600u,   800u,   1000u,  1500u,  2000u,
};
static const ParamValueByCoresCount<uint32_t, g_max_cores_count> g_textures_count = {
    5u,   10u,   20u,   30u,   40u,   50u,   60u,    70u,    80u,    90u,    100u
};
static const ParamValueByCoresCount<float, g_max_cores_count> g_scale_ratio = {
    0.6f, 0.55f, 0.5f,  0.45f, 0.4f,  0.35f, 0.3f,   0.25f,  0.2f,   0.15f,  0.1f
};

// Common application settings
static const std::string           g_app_help_text  = "Asteroids sample demonstrates parallel rendering of the asteroids field observable with interactive camera.";
static const GraphicsApp::Settings  g_app_settings  = // Application settings:
{                                                     // ====================
    {                                                 // app:
        "Methane Asteroids",                          // - name
        0.8, 0.8,                                     // - width, height
        false,                                        // - is_full_screen
    },                                                //
    {                                                 // context:
        gfx::FrameSize(),                             // - frame_size
        gfx::PixelFormat::BGRA8Unorm,                 // - color_format
        gfx::PixelFormat::Depth32Float,               // - depth_stencil_format
        { /* color clearing disabled */ },            // - clear_color
        gfx::DepthStencil{ 0.f, 0u },                 // - clear_depth_stencil
        3u,                                           // - frame_buffers_count
        true,                                         // - vsync_enabled
    },                                                //
    true                                              // show_hud_in_window_title
};

AsteroidsApp::AsteroidsApp()
    : GraphicsApp(g_app_settings, gfx::RenderPass::Access::ShaderResources | gfx::RenderPass::Access::Samplers, g_app_help_text)
    , m_view_camera(m_animations, gfx::ActionCamera::Pivot::Aim)
    , m_light_camera(m_view_camera, m_animations, gfx::ActionCamera::Pivot::Aim)
    , m_scene_scale(15.f)
    , m_scene_constants(                                    // Shader constants:
        {                                                   // ================
            gfx::Color4f(1.f, 1.f, 1.f, 1.f),               // - light_color
            1.25f,                                          // - light_power
            0.1f,                                           // - light_ambient_factor
            4.f                                             // - light_specular_factor
        })
    , m_asteroids_array_settings(                           // Asteroids array settings:
        {                                                   // ================
            m_view_camera,                                  // - view_camera
            m_scene_scale,                                  // - scale
            GetParamValueByCoresCount(g_instaces_count),    // - instance_count
            GetParamValueByCoresCount(g_mesh_count),        // - unique_mesh_count
            1u,                                             // - minimum_subdivision
            3u,                                             // - subdivisions_count
            GetParamValueByCoresCount(g_textures_count),    // - textures_count
            { 256u, 256u },                                 // - texture_dimensions
            1123u,                                          // - random_seed
            13.f,                                           // - orbit_radius_ratio
            4.f,                                            // - disc_radius_ratio
            GetParamValueByCoresCount(g_scale_ratio) / 10.f,// - min_asteroid_scale_ratio
            GetParamValueByCoresCount(g_scale_ratio),       // - max_asteroid_scale_ratio
            true                                            // - depth_reversed
        })
{
    ITT_FUNCTION_TASK();

    // NOTE: Near and Far values are swapped in camera parameters (1st value is near = max depth, 2nd value is far = min depth)
    // for Reversed-Z buffer values range [ near: 1, far 0], instead of [ near 0, far 1]
    // which is used for "from near to far" drawing order for reducing pixels overdraw
    m_view_camera.SetOrientation({ { 0.f, 60.f, 250.f }, { 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f } });
    m_view_camera.SetParamters({ 600.f /* near = max depth */, 0.01f /*far = min depth*/, 90.f /* FOV */ });
    m_view_camera.SetZoomDistanceRange({ 60.f , 400.f });

    m_light_camera.SetOrientation({ { -100.f, 120.f, 0.f }, { 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f } });
    m_light_camera.SetProjection(gfx::Camera::Projection::Orthogonal);
    m_light_camera.SetParamters({ -300.f, 300.f, 90.f });
    m_light_camera.Resize(120.f, 120.f);

    m_input_state.AddControllers({
        std::make_shared<gfx::AppCameraController>(m_view_camera,  "VIEW CAMERA"),
        std::make_shared<gfx::AppCameraController>(m_light_camera, "LIGHT SOURCE",
            gfx::AppCameraController::ActionByMouseButton   { { pal::Mouse::Button::Right, gfx::ActionCamera::MouseAction::Rotate   } },
            gfx::AppCameraController::ActionByKeyboardState { { { pal::Keyboard::Key::L }, gfx::ActionCamera::KeyboardAction::Reset } },
            gfx::AppCameraController::ActionByKeyboardKey   { }),
    });
}

AsteroidsApp::~AsteroidsApp()
{
    ITT_FUNCTION_TASK();
    // Wait for GPU rendering is completed to release resources
    m_sp_context->WaitForGpu(gfx::Context::WaitFor::RenderComplete);
}

void AsteroidsApp::Init()
{
    ITT_FUNCTION_TASK();

    GraphicsApp::Init();

    assert(m_sp_context);
    gfx::Context& context = *m_sp_context;

    const gfx::Context::Settings& context_settings = context.GetSettings();
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
        true,                           // mipmapped
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
#if PARALLEL_RENDERING_ENABLED
        frame.sp_parallel_cmd_list = gfx::ParallelRenderCommandList::Create(context.GetRenderCommandQueue(), *frame.sp_screen_pass);
        frame.sp_parallel_cmd_list->SetParallelCommandListsCount(std::thread::hardware_concurrency());
        frame.sp_parallel_cmd_list->SetName(IndexedName("Parallel Rendering", frame.index));
#else
        // Create command list for final FB rendering
        frame.sp_cmd_list = gfx::RenderCommandList::Create(context.GetRenderCommandQueue(), *frame.sp_screen_pass);
        frame.sp_cmd_list->SetName(IndexedName("Scene Rendering", frame.index));
#endif

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
        frame.skybox.resource_bindings_per_instance.resize(1);
        frame.skybox.resource_bindings_per_instance[0] = m_sp_sky_box->CreateResourceBindings(frame.skybox.sp_uniforms_buffer);

        // Resource bindings for Planet rendering
        frame.planet.resource_bindings_per_instance.resize(1);
        frame.planet.resource_bindings_per_instance[0] = m_sp_planet->CreateResourceBindings(m_sp_const_buffer, frame.planet.sp_uniforms_buffer);

        // Resource bindings for Asteroids rendering
        frame.asteroids.resource_bindings_per_instance = m_sp_asteroids_array->CreateResourceBindings(m_sp_const_buffer, frame.sp_scene_uniforms_buffer, frame.asteroids.sp_uniforms_buffer);
    }

    // Setup animations
    m_animations.push_back(std::make_shared<Data::TimeAnimation>(std::bind(&Planet::Update, m_sp_planet.get(), std::placeholders::_1, std::placeholders::_2)));
    m_animations.push_back(std::make_shared<Data::TimeAnimation>(std::bind(&AsteroidsArray::Update, m_sp_asteroids_array.get(), std::placeholders::_1, std::placeholders::_2)));

    // Complete initialization of render context:
    //  - allocate deferred descriptor heaps with calculated sizes
    //  - execute commands to upload resources to GPU
    context.CompleteInitialization();
}

bool AsteroidsApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    ITT_FUNCTION_TASK();

    // Resize screen color and depth textures
    if (!GraphicsApp::Resize(frame_size, is_minimized))
        return false;

    m_sp_sky_box->Resize(frame_size);
    m_sp_planet->Resize(frame_size);
    m_sp_asteroids_array->Resize(frame_size);

    m_view_camera.Resize(static_cast<float>(frame_size.width), static_cast<float>(frame_size.height));
    return true;
}

bool AsteroidsApp::Update()
{
    ITT_FUNCTION_TASK();

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

    // Render only when context is ready
    assert(!!m_sp_context);
    if (!m_sp_context->ReadyToRender() || !GraphicsApp::Render())
        return false;

    // Wait for previous frame rendering is completed and switch to next frame
    m_sp_context->WaitForGpu(gfx::Context::WaitFor::FramePresented);
    AsteroidsFrame& frame = GetCurrentFrame();

    // Upload uniform buffers to GPU
    frame.sp_scene_uniforms_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&m_scene_uniforms), sizeof(SceneUniforms) } });

    // Asteroids rendering
    assert(!!m_sp_asteroids_array);

#if PARALLEL_RENDERING_ENABLED
    assert(!!frame.sp_parallel_cmd_list);
    m_sp_asteroids_array->Draw(*frame.sp_parallel_cmd_list, frame.asteroids);
    gfx::RenderCommandList& render_cmd_list = *frame.sp_parallel_cmd_list->GetParallelCommandLists().back();
    gfx::CommandList&       commit_cmd_list = *frame.sp_parallel_cmd_list;
#else
    assert(!!frame.sp_cmd_list);
    m_sp_asteroids_array->Draw(*frame.sp_cmd_list, frame.asteroids);
    gfx::RenderCommandList& render_cmd_list = *frame.sp_cmd_list;
    gfx::CommandList&       commit_cmd_list = *frame.sp_cmd_list;
#endif
    
    render_cmd_list.PopDebugGroup();
    
    // Planet rendering
    assert(!!m_sp_planet);
    m_sp_planet->Draw(render_cmd_list, frame.planet);

    // Sky-box rendering
    assert(!!m_sp_sky_box);
    m_sp_sky_box->Draw(render_cmd_list, frame.skybox);
    
    commit_cmd_list.Commit(true);

    // Execute rendering commands and present frame to screen
    m_sp_context->GetRenderCommandQueue().Execute({
#if PARALLEL_RENDERING_ENABLED
        *frame.sp_parallel_cmd_list,
#else
        *frame.sp_cmd_list,
#endif
    });
    m_sp_context->Present();

    return true;
}

void AsteroidsApp::OnContextReleased()
{
    ITT_FUNCTION_TASK();

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

} // namespace Methane::Samples

int main(int argc, const char* argv[])
{
    return Methane::Samples::AsteroidsApp().Run({ argc, argv });
}
