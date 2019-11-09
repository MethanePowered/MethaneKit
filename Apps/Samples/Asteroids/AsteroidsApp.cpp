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

#define _ENABLE_EXTENDED_ALIGNED_STORAGE // FIXME: get rid of this ASAP

#include "AsteroidsApp.h"

#include <Methane/Graphics/AppCameraController.h>
#include <Methane/Data/TimeAnimation.h>

#include <cml/mathlib/mathlib.h>
#include <cassert>
#include <memory>

namespace Methane::Samples
{

// Common application settings
static constexpr uint32_t           g_asteroid_instances_count    = 10;
static constexpr uint32_t           g_asteroid_subdivisions_count = 3;
static const std::string            g_app_help_text = "Asteroids sample demonstrates parallel rendering of multiple heterogeneous objects " \
                                                      "and action camera interaction with mouse and keyboard.";
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
        gfx::Color(0.0f, 0.2f, 0.4f, 1.0f),           // - clear_color
        1.f,                                          // - clear_depth
        0,                                            // - clear_stencil
        3,                                            // - frame_buffers_count
        true,                                         // - vsync_enabled
    },                                                //
    true                                              // show_hud_in_window_title
};

AsteroidsApp::AsteroidsApp()
    : GraphicsApp(g_app_settings, gfx::RenderPass::Access::ShaderResources | gfx::RenderPass::Access::Samplers, g_app_help_text)
    , m_scene_scale(15.f)
    , m_scene_constants(                              // Shader constants:
        {                                             // ================
            gfx::Color(1.f, 1.f, 0.74f, 1.f),         // - light_color
            10000.f,                                  // - light_power
            0.1f,                                     // - light_ambient_factor
            4.f                                       // - light_specular_factor
        })
    , m_view_camera(m_animations, gfx::ActionCamera::Pivot::Aim)
    , m_light_camera(m_view_camera, m_animations, gfx::ActionCamera::Pivot::Aim)
{
    m_view_camera.SetOrientation({ { 0.f, 40.f, 90.f }, { 0.f, -25.f, 0.f }, { 0.f, 1.f, 0.f } });
    m_view_camera.SetParamters({ 0.01f, 600.f, 90.f });
    m_view_camera.SetZoomDistanceRange({ 15.f , 300.f });

    m_light_camera.SetOrientation({ { 100.f,  100.f, 100.f }, { 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f } });
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
    // Wait for GPU rendering is completed to release resources
    m_sp_context->WaitForGpu(gfx::Context::WaitFor::RenderComplete);
}

void AsteroidsApp::Init()
{
    GraphicsApp::Init();

    assert(m_sp_context);
    gfx::Context& context = *m_sp_context;

    const gfx::Context::Settings& context_settings = context.GetSettings();
    m_view_camera.Resize(static_cast<float>(context_settings.frame_size.width),
                         static_cast<float>(context_settings.frame_size.height));

    // Create sky-box
    m_sp_sky_box = std::make_shared<gfx::SkyBox>(context, m_image_loader, gfx::SkyBox::Settings{
        m_scene_scale * 100.f,
        m_view_camera,
        {
            "Textures/SkyBox/Galaxy/PositiveX.jpg",
            "Textures/SkyBox/Galaxy/NegativeX.jpg",
            "Textures/SkyBox/Galaxy/PositiveY.jpg",
            "Textures/SkyBox/Galaxy/NegativeY.jpg",
            "Textures/SkyBox/Galaxy/PositiveZ.jpg",
            "Textures/SkyBox/Galaxy/NegativeZ.jpg"
        }
    });

    // Create vertex and index buffer for meshes
    m_sp_asteroid_array = std::make_unique<AsteroidArray>(context, AsteroidArray::Settings{
        m_view_camera,
        g_asteroid_instances_count,
        g_asteroid_subdivisions_count,
        m_scene_scale
    });
    m_sp_asteroid_array->SetTexture(m_image_loader.LoadImageToTexture2D(context, "Textures/MethaneBubbles.jpg"));

    const Data::Size constants_data_size         = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(Constants)));
    const Data::Size scene_uniforms_data_size    = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(SceneUniforms)));
    const Data::Size asteroid_uniforms_data_size = m_sp_asteroid_array->GetUniformsBufferSize();

    // Create constants buffer for frame rendering
    m_sp_const_buffer = gfx::Buffer::CreateConstantBuffer(context, constants_data_size);
    m_sp_const_buffer->SetName("Constants Buffer");
    m_sp_const_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&m_scene_constants), sizeof(m_scene_constants) } });

    // Create sampler for image texture
    m_sp_texture_sampler = gfx::Sampler::Create(context, {
        { gfx::Sampler::Filter::MinMag::Linear     },    // Bilinear filtering
        { gfx::Sampler::Address::Mode::ClampToZero }
    });
    m_sp_texture_sampler->SetName("Texture Sampler");

    // Create state for final FB rendering with a program
    gfx::RenderState::Settings state_settings;
    state_settings.sp_program = gfx::Program::Create(context, {
        {
            gfx::Shader::CreateVertex(context, { Data::ShaderProvider::Get(), { "Asteroids", "AsteroidsVS" }, { } }),
            gfx::Shader::CreatePixel( context, { Data::ShaderProvider::Get(), { "Asteroids", "AsteroidsPS" }, { } }),
        },
        { // input_buffer_layouts
            { // Single vertex buffer with interleaved data:
                { // input arguments mapping to semantic names
                    { "in_position", "POSITION" },
                    { "in_normal",   "NORMAL"   },
                    { "in_uv",       "TEXCOORD" },
                }
            }
        },
        { // constant_argument_names
            "g_constants", "g_texture_sampler"
        },
        { // addressable_argument_names
            "g_mesh_uniforms"
        },
        { // render_target_pixel_formats
            context_settings.color_format
        },
        context_settings.depth_stencil_format
    });
    state_settings.sp_program->SetName("Textured Phong Lighting");
    state_settings.viewports     = { gfx::GetFrameViewport(context_settings.frame_size) };
    state_settings.scissor_rects = { gfx::GetFrameScissorRect(context_settings.frame_size) };
    state_settings.depth.enabled = true;
    
    m_sp_state = gfx::RenderState::Create(context, state_settings);
    m_sp_state->SetName("Final FB render state");

    // ========= Per-Frame Data =========
    for(AsteroidsFrame& frame : m_frames)
    {
        // Create render pass and command list for final pass rendering
        frame.sp_cmd_list = gfx::RenderCommandList::Create(context.GetRenderCommandQueue(), *frame.sp_screen_pass);
        frame.sp_cmd_list->SetName(IndexedName("Scene Rendering", frame.index));

        // Create uniforms buffer with volatile parameters for the whole scene rendering
        frame.sp_scene_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(context, scene_uniforms_data_size);
        frame.sp_scene_uniforms_buffer->SetName(IndexedName("Scene Uniforms Buffer", frame.index));

        // Create uniforms buffer for Sky-Box rendering
        frame.skybox.sp_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(context, sizeof(gfx::SkyBox::MeshUniforms));
        frame.skybox.sp_uniforms_buffer->SetName(IndexedName("Sky-box Uniforms Buffer", frame.index));

        // Resource bindings for Sky-Box rendering
        frame.skybox.resource_bindings_array.resize(1);
        frame.skybox.resource_bindings_array[0] = m_sp_sky_box->CreateResourceBindings(frame.skybox.sp_uniforms_buffer);

        // Create uniforms buffer for Cube rendering
        frame.asteroids.sp_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(context, asteroid_uniforms_data_size);
        frame.asteroids.sp_uniforms_buffer->SetName(IndexedName("Cube Uniforms Buffer", frame.index));

        // Resource bindings for cube rendering
        const uint32_t asteroids_count = m_sp_asteroid_array->GetSubsetsCount();
        frame.asteroids.resource_bindings_array.resize(asteroids_count);
        if (asteroids_count == 0)
            continue;

        frame.asteroids.resource_bindings_array[0] = gfx::Program::ResourceBindings::Create(state_settings.sp_program, {
            { { gfx::Shader::Type::Vertex, "g_mesh_uniforms"  }, { frame.asteroids.sp_uniforms_buffer, m_sp_asteroid_array->GetUniformsBufferOffset(0) } },
            { { gfx::Shader::Type::Pixel,  "g_scene_uniforms" }, { frame.sp_scene_uniforms_buffer        } },
            { { gfx::Shader::Type::Pixel,  "g_constants"      }, { m_sp_const_buffer                     } },
            { { gfx::Shader::Type::Pixel,  "g_texture"        }, { m_sp_asteroid_array->GetTexturePtr()  } },
            { { gfx::Shader::Type::Pixel,  "g_texture_sampler"}, { m_sp_texture_sampler                  } },
        });

        for(uint32_t asteroid_index = 1; asteroid_index < asteroids_count; ++asteroid_index)
        {
            const Data::Size asteroid_uniform_offset = m_sp_asteroid_array->GetUniformsBufferOffset(asteroid_index);
            frame.asteroids.resource_bindings_array[asteroid_index] = gfx::Program::ResourceBindings::CreateCopy(*frame.asteroids.resource_bindings_array[0], {
                { { gfx::Shader::Type::Vertex, "g_mesh_uniforms"  }, { frame.asteroids.sp_uniforms_buffer, asteroid_uniform_offset } },
            });
        }
    }

    // Setup animations
    m_animations.push_back(std::make_shared<Data::TimeAnimation>(std::bind(&AsteroidArray::Update, m_sp_asteroid_array.get(), std::placeholders::_1, std::placeholders::_2)));

    // Complete initialization of render context:
    //  - allocate deferred descriptor heaps with calculated sizes
    //  - execute commands to upload resources to GPU
    context.CompleteInitialization();
}

bool AsteroidsApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    if (!m_initialized || GetInitialContextSettings().frame_size == frame_size)
        return false;

    // Resize screen color and depth textures
    GraphicsApp::Resize(frame_size, is_minimized);

    // Update viewports and scissor rects state
    assert(m_sp_state);
    m_sp_state->SetViewports({ gfx::GetFrameViewport(frame_size) });
    m_sp_state->SetScissorRects({ gfx::GetFrameScissorRect(frame_size) });
    m_sp_sky_box->Resize(frame_size);

    m_view_camera.Resize(static_cast<float>(frame_size.width), static_cast<float>(frame_size.height));
    return true;
}

void AsteroidsApp::Update()
{
    GraphicsApp::Update();

    // Update scene uniforms
    m_scene_uniforms.eye_position    = gfx::Vector4f(m_view_camera.GetOrientation().eye, 1.f);
    m_scene_uniforms.light_position  = m_light_camera.GetOrientation().eye;

    m_sp_sky_box->Update();

    //m_sp_asteroid_array->Update(0., 0.);
}

void AsteroidsApp::Render()
{
    // Render only when context is ready
    assert(!!m_sp_context);
    if (!m_sp_context->ReadyToRender())
        return;

    // Wait for previous frame rendering is completed and switch to next frame
    m_sp_context->WaitForGpu(gfx::Context::WaitFor::FramePresented);
    AsteroidsFrame& frame = GetCurrentFrame();

    // Upload uniform buffers to GPU
    frame.sp_scene_uniforms_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&m_scene_uniforms), sizeof(SceneUniforms) } });
    frame.asteroids.sp_uniforms_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&m_sp_asteroid_array->GetFinalPassUniforms()),
                                                    m_sp_asteroid_array->GetUniformsBufferSize() } });

    // Sky-box drawing
    assert(!!frame.sp_cmd_list);
    assert(!!frame.skybox.sp_uniforms_buffer);
    assert(!!frame.skybox.resource_bindings_array[0]);
    assert(!!m_sp_sky_box);
    m_sp_sky_box->Draw(*frame.sp_cmd_list, *frame.skybox.sp_uniforms_buffer, *frame.skybox.resource_bindings_array[0]);

    // Cube drawing
    frame.sp_cmd_list->Reset(*m_sp_state, "Asteroids rendering");

    assert(!!m_sp_asteroid_array);
    m_sp_asteroid_array->Draw(*frame.sp_cmd_list, frame.asteroids.resource_bindings_array);

    frame.sp_cmd_list->Commit(true);

    // Execute rendering commands and present frame to screen
    m_sp_context->GetRenderCommandQueue().Execute({
        *frame.sp_cmd_list,
    });
    m_sp_context->Present();

    GraphicsApp::Render();
}

void AsteroidsApp::OnContextReleased()
{
    m_animations.clear();
    m_sp_sky_box.reset();
    m_sp_asteroid_array.reset();
    m_sp_texture_sampler.reset();
    m_sp_const_buffer.reset();
    m_sp_state.reset();

    GraphicsApp::OnContextReleased();
}

} // namespace Methane::Samples

int main(int argc, const char* argv[])
{
    return Methane::Samples::AsteroidsApp().Run({ argc, argv });
}
