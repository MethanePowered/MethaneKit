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

FILE: TexturedCubeApp.cpp
Tutorial demonstrating textured cube rendering with Methane graphics API

******************************************************************************/

#include "TexturedCubeApp.h"

#include <cml/mathlib/mathlib.h>

#include <cassert>

using namespace Methane::Tutorials;

static const gfx::Shader::EntryTarget g_vs_main   = { "VSMain", "vs_5_1" };
static const gfx::Shader::EntryTarget g_ps_main   = { "PSMain", "ps_5_1" };
static const GraphicsApp::Settings g_app_settings = // Application settings:
{                                                   // ====================
    {                                               // app:
        "Methane Textured Cube",                    // - name
        0.8, 0.8,                                   // - width, height
    },                                              //
    {                                               // context:
        gfx::FrameSize(),                           // - frame_size
        gfx::PixelFormat::BGRA8Unorm,               // - color_format
        gfx::PixelFormat::Depth32Float,             // - depth_stencil_format
        gfx::Color(0.0f, 0.2f, 0.4f, 1.0f),         // - clear_color
        1.f,                                        // - clear_depth
        0,                                          // - clear_stencil
        3,                                          // - frame_buffers_count
        true,                                       // - vsync_enabled
    },                                              //
    true                                            // show_hud_in_window_title
};

TexturedCubeApp::TexturedCubeApp()
    : GraphicsApp(g_app_settings, gfx::RenderPass::Access::ShaderResources | gfx::RenderPass::Access::Samplers)
    , m_shader_constants(                           // Shader constants:
        {                                           // ================
            gfx::Color(1.f, 1.f, 0.74f, 1.f),       // - light_color
            700.f,                                  // - light_power
            0.2f,                                   // - light_ambient_factor
            5.f                                     // - light_specular_factor
        })
    , m_cube_mesh(gfx::Mesh::VertexLayoutFromArray(Vertex::layout))
    , m_cube_scale(15.f)
{
    m_shader_uniforms.light_position = gfx::Vector3f(0.f, 20.f, -25.f);
    m_camera.SetOrientation({ { 13.0f, 13.0f, -13.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } });
}

TexturedCubeApp::~TexturedCubeApp()
{
    // Wait for GPU rendering is completed to release resources
    m_sp_context->WaitForGpu(gfx::Context::WaitFor::RenderComplete);
}

void TexturedCubeApp::Init()
{
    GraphicsApp::Init();

    assert(m_sp_context);
    const gfx::Context::Settings& context_settings = m_sp_context->GetSettings();
    m_camera.Resize(static_cast<float>(context_settings.frame_size.width),
                    static_cast<float>(context_settings.frame_size.height));

    // Create cube shading program
    m_sp_program = gfx::Program::Create(*m_sp_context, {
        { // shaders
            gfx::Shader::CreateVertex(*m_sp_context, { g_vs_main }),
            gfx::Shader::CreatePixel( *m_sp_context, { g_ps_main }),
        },
        { // input_buffer_layouts
            { // single vertex buffer layout with interleaved data
                { // input arguments mapping to semantic names
                    { "in_position", "POSITION" },
                    { "in_normal",   "NORMAL"   },
                    { "in_uv",       "TEXCOORD" },
                }
            }
        },
        { // constant_argument_names
            "g_constants", "g_texture", "g_sampler"
        },
        { // render_target_pixel_formats
            context_settings.color_format
        },
        context_settings.depth_stencil_format
    });
    m_sp_program->SetName("Textured Phong Lighting");

    // Load texture image from file
    m_sp_cube_texture = m_image_loader.CreateImageTexture(*m_sp_context, "Textures/MethaneBubbles.jpg");
    m_sp_cube_texture->SetName("Cube Texture 2D Image");

    // Create sampler for image texture
    m_sp_texture_sampler = gfx::Sampler::Create(*m_sp_context, {
        { gfx::Sampler::Filter::MinMag::Linear     },    // Bilinear filtering
        { gfx::Sampler::Address::Mode::ClampToZero }
     });

    const Data::Size constants_data_size = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(m_shader_constants)));
    const Data::Size uniforms_data_size  = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(m_shader_uniforms)));

    // Create constants buffer for frame rendering
    m_sp_const_buffer = gfx::Buffer::CreateConstantBuffer(*m_sp_context, constants_data_size);
    m_sp_const_buffer->SetName("Constants Buffer");
    m_sp_const_buffer->SetData(reinterpret_cast<Data::ConstRawPtr>(&m_shader_constants), sizeof(m_shader_constants));

    // Create frame buffer data
    for(TexturedCubeFrame& frame : m_frames)
    {
        // Create uniforms buffer with volatile parameters for frame rendering
        frame.sp_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(*m_sp_context, uniforms_data_size);
        frame.sp_uniforms_buffer->SetName(IndexedName("Uniforms Buffer", frame.index));

        // Configure program resource bindings
        frame.sp_resource_bindings = gfx::Program::ResourceBindings::Create(m_sp_program, {
            { { gfx::Shader::Type::All,   "g_uniforms"  }, frame.sp_uniforms_buffer  },
            { { gfx::Shader::Type::Pixel, "g_constants" }, m_sp_const_buffer         },
            { { gfx::Shader::Type::Pixel, "g_texture"   }, m_sp_cube_texture         },
            { { gfx::Shader::Type::Pixel, "g_sampler"   }, m_sp_texture_sampler      },
        });
        
        // Create command list for rendering
        frame.sp_cmd_list = gfx::RenderCommandList::Create(m_sp_context->GetRenderCommandQueue(), *frame.sp_screen_pass);
        frame.sp_cmd_list->SetName(IndexedName("Cube Rendering", frame.index));
    }

    // Create vertex buffer for cube mesh
    const Data::Size vertex_data_size = static_cast<Data::Size>(m_cube_mesh.GetVertexDataSize());
    const Data::Size vertex_size      = static_cast<Data::Size>(m_cube_mesh.GetVertexSize());
    m_sp_vertex_buffer = gfx::Buffer::CreateVertexBuffer(*m_sp_context, vertex_data_size, vertex_size);
    m_sp_vertex_buffer->SetName("Cube Vertex Buffer");
    m_sp_vertex_buffer->SetData(reinterpret_cast<Data::ConstRawPtr>(m_cube_mesh.GetVertices().data()), vertex_data_size);

    // Create index buffer for cube mesh
    const Data::Size index_data_size = static_cast<Data::Size>(m_cube_mesh.GetIndexDataSize());
    m_sp_index_buffer  = gfx::Buffer::CreateIndexBuffer(*m_sp_context, index_data_size, gfx::PixelFormat::R32Uint);
    m_sp_index_buffer->SetName("Cube Index Buffer");
    m_sp_index_buffer->SetData(reinterpret_cast<Data::ConstRawPtr>(m_cube_mesh.GetIndices().data()), index_data_size);

    // Create render state
    gfx::RenderState::Settings state_settings;
    state_settings.sp_program    = m_sp_program;
    state_settings.viewports     = { gfx::GetFrameViewport(context_settings.frame_size) };
    state_settings.scissor_rects = { gfx::GetFrameScissorRect(context_settings.frame_size) };
    state_settings.depth.enabled = true;
    m_sp_state = gfx::RenderState::Create(*m_sp_context, state_settings);
    m_sp_state->SetName("Final FB Render Pipeline State");

    // Complete initialization of render context:
    //  - allocate deferred descriptor heaps with calculated sizes
    //  - execute commands to upload resources to GPU
    m_sp_context->CompleteInitialization();
}

bool TexturedCubeApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    // Resize screen color and depth textures
    if (!GraphicsApp::Resize(frame_size, is_minimized))
        return false;

    // Update viewports and scissor rects state
    assert(m_sp_state);
    m_sp_state->SetViewports({ gfx::GetFrameViewport(frame_size) });
    m_sp_state->SetScissorRects({ gfx::GetFrameScissorRect(frame_size) });

    m_camera.Resize(static_cast<float>(frame_size.width), static_cast<float>(frame_size.height));
    return true;
}

void TexturedCubeApp::Update()
{
    if (HasError())
        return;

    const float elapsed_time_sec = m_timer.GetElapsedSecondsF();
    m_timer.Reset();

    // Update Model, View, Projection matrices based on camera location
    gfx::Matrix44f model_matrix, view_matrix, proj_matrix;
    cml::matrix_uniform_scale(model_matrix, m_cube_scale);

    m_camera.RotateYaw(360.f * elapsed_time_sec / 8.f);
    m_camera.GetViewProjMatrices(view_matrix, proj_matrix);

    gfx::Matrix33f light_rotate_matrix;
    cml::matrix_rotation_axis_angle(light_rotate_matrix, m_camera.GetOrientation().up, cml::rad(360.f * elapsed_time_sec / 4.f));

    gfx::Matrix44f mv_matrix         = model_matrix * view_matrix;
    m_shader_uniforms.mvp_matrix     = mv_matrix * proj_matrix;
    m_shader_uniforms.model_matrix   = model_matrix;
    m_shader_uniforms.eye_position   = gfx::Vector4f(m_camera.GetOrientation().eye, 1.f);
    m_shader_uniforms.light_position = m_shader_uniforms.light_position * light_rotate_matrix;

    // Update constant buffer related to current frame
    TexturedCubeFrame& frame = GetCurrentFrame();
    assert(!!frame.sp_uniforms_buffer);
    frame.sp_uniforms_buffer->SetData(reinterpret_cast<Data::ConstRawPtr>(&m_shader_uniforms), sizeof(m_shader_uniforms));
}

void TexturedCubeApp::Render()
{
    // Do not render if error has occured and is being displayed in message box
    if (HasError())
        return;

    // Render only when context is ready
    assert(!!m_sp_context);
    if (!m_sp_context->ReadyToRender())
        return;

    // Wait for previous frame rendering is completed and switch to next frame
    m_sp_context->WaitForGpu(gfx::Context::WaitFor::FramePresented);

    TexturedCubeFrame& frame = GetCurrentFrame();
    assert(!!frame.sp_cmd_list);
    assert(!!frame.sp_resource_bindings);
    assert(!!m_sp_vertex_buffer);
    assert(!!m_sp_index_buffer);
    assert(!!m_sp_state);

    // Issue commands for cube rendering
    frame.sp_cmd_list->Reset(*m_sp_state, "Cube redering");
    frame.sp_cmd_list->SetResourceBindings(*frame.sp_resource_bindings);
    frame.sp_cmd_list->SetVertexBuffers({ *m_sp_vertex_buffer });
    frame.sp_cmd_list->DrawIndexed(gfx::RenderCommandList::Primitive::Triangle, *m_sp_index_buffer, 1);
    frame.sp_cmd_list->Commit(true);

    // Present frame to screen
    m_sp_context->GetRenderCommandQueue().Execute({ *frame.sp_cmd_list });
    m_sp_context->Present();

    GraphicsApp::Render();
}

int main(int argc, const char* argv[])
{
    return TexturedCubeApp().Run({ argc, argv });
}
