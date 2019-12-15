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
#include <thread>

#define PARALLEL_RENDERING_ENABLED 1

namespace Methane::Samples
{

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
        gfx::Color4f(0.0f, 0.0f, 0.0f, 1.0f),         // - clear_color
        1.f,                                          // - clear_depth
        0,                                            // - clear_stencil
        3,                                            // - frame_buffers_count
        true,                                         // - vsync_enabled
    },                                                //
    true                                              // show_hud_in_window_title
};

AsteroidsApp::AsteroidsApp()
    : GraphicsApp(g_app_settings, gfx::RenderPass::Access::ShaderResources | gfx::RenderPass::Access::Samplers, g_app_help_text)
    , m_view_camera(m_animations, gfx::ActionCamera::Pivot::Aim)
    , m_light_camera(m_view_camera, m_animations, gfx::ActionCamera::Pivot::Aim)
    , m_scene_scale(15.f)
    , m_scene_constants(                              // Shader constants:
        {                                             // ================
            gfx::Color4f(1.f, 1.f, 1.f, 1.f),         // - light_color
            1.25f,                                    // - light_power
            0.1f,                                     // - light_ambient_factor
            4.f                                       // - light_specular_factor
        })
    , m_asteroids_array_settings(                     // Asteroids array settings:
        {                                             // ================
            m_view_camera,                            // - view_camera
            m_scene_scale,                            // - scale
            10000u,                                   // - instance_count
            700u,                                     // - unique_mesh_count
            3u,                                       // - subdivisions_count
            30u,                                      // - textures_count
            { 256u, 256u },                           // - texture_dimensions
            1123u,                                    // - random_seed
            13.f,                                     // - orbit_radius_ratio
            4.f,                                      // - disc_radius_ratio
            0.03f,                                   // - min_asteroid_scale_ratio
            0.3f,                                    // - max_asteroid_scale_ratio
        })
{
    ITT_FUNCTION_TASK();

    m_view_camera.SetOrientation({ { 0.f, 60.f, 250.f }, { 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f } });
    m_view_camera.SetParamters({ 0.01f, 600.f, 90.f });
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
        m_scene_scale * 100.f
    });

    // Create planet
    m_sp_planet = std::make_shared<Planet>(context, m_image_loader, Planet::Settings{
        m_view_camera,
        m_light_camera,
        "Textures/Planet/Mars.jpg",
        gfx::Vector3f(0.f, 0.f, 0.f),
        m_scene_scale * 3.f,
        0.1f,
        true
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
        // Create command list for final FB rendering
        frame.sp_cmd_list = gfx::RenderCommandList::Create(context.GetRenderCommandQueue(), *frame.sp_screen_pass);
        frame.sp_cmd_list->SetName(IndexedName("Scene Rendering", frame.index));

#if PARALLEL_RENDERING_ENABLED
        gfx::RenderPass::Settings asteroids_render_pass_settings = frame.sp_screen_pass->GetSettings();
        asteroids_render_pass_settings.color_attachments[0].load_action = gfx::RenderPass::Attachment::LoadAction::Load;
        asteroids_render_pass_settings.depth_attachment.load_action     = gfx::RenderPass::Attachment::LoadAction::Load;
        frame.sp_asteroids_render_pass = gfx::RenderPass::Create(*m_sp_context, asteroids_render_pass_settings);

        frame.sp_parallel_cmd_list = gfx::ParallelRenderCommandList::Create(context.GetRenderCommandQueue(), *frame.sp_asteroids_render_pass);
        frame.sp_parallel_cmd_list->SetParallelCommandListsCount(std::thread::hardware_concurrency());
        frame.sp_parallel_cmd_list->SetName(IndexedName("Parallel Asteroids Rendering", frame.index));
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

    if (!m_initialized || GetInitialContextSettings().frame_size == frame_size)
        return false;

    // Resize screen color and depth textures
    GraphicsApp::Resize(frame_size, is_minimized);

    m_sp_sky_box->Resize(frame_size);
    m_sp_planet->Resize(frame_size);
    m_sp_asteroids_array->Resize(frame_size);

    m_view_camera.Resize(static_cast<float>(frame_size.width), static_cast<float>(frame_size.height));
    return true;
}

void AsteroidsApp::Update()
{
    ITT_FUNCTION_TASK();

    GraphicsApp::Update();

    // Update scene uniforms
    m_scene_uniforms.eye_position    = gfx::Vector4f(m_view_camera.GetOrientation().eye, 1.f);
    m_scene_uniforms.light_position  = m_light_camera.GetOrientation().eye;

    m_sp_sky_box->Update();
}

void AsteroidsApp::Render()
{
    ITT_FUNCTION_TASK();

    // Render only when context is ready
    assert(!!m_sp_context);
    if (!m_sp_context->ReadyToRender())
        return;

    // Wait for previous frame rendering is completed and switch to next frame
    m_sp_context->WaitForGpu(gfx::Context::WaitFor::FramePresented);
    AsteroidsFrame& frame = GetCurrentFrame();

    // Upload uniform buffers to GPU
    frame.sp_scene_uniforms_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&m_scene_uniforms), sizeof(SceneUniforms) } });

    assert(!!frame.sp_cmd_list);

    // Sky-box rendering
    assert(!!m_sp_sky_box);
    m_sp_sky_box->Draw(*frame.sp_cmd_list, frame.skybox);

    // Planet rendering
    assert(!!m_sp_planet);
    m_sp_planet->Draw(*frame.sp_cmd_list, frame.planet);

    // Asteroids rendering
    assert(!!m_sp_asteroids_array);

#if PARALLEL_RENDERING_ENABLED
    frame.sp_cmd_list->Commit(false);

    assert(!!frame.sp_parallel_cmd_list);
    m_sp_asteroids_array->Draw(*frame.sp_parallel_cmd_list, frame.asteroids);
    frame.sp_parallel_cmd_list->Commit(true);
#else
    m_sp_asteroids_array->Draw(*frame.sp_cmd_list, frame.asteroids);
    frame.sp_cmd_list->Commit(true);
#endif

    // Execute rendering commands and present frame to screen
    m_sp_context->GetRenderCommandQueue().Execute({
        *frame.sp_cmd_list,
#if PARALLEL_RENDERING_ENABLED
        *frame.sp_parallel_cmd_list,
#endif
    });
    m_sp_context->Present();

    GraphicsApp::Render();
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
