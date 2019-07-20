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
Sample demonstrating parallel redering of the distinct asteroids massive

******************************************************************************/

#include "AsteroidsApp.h"
#include <Methane/Graphics/AppCameraController.hpp>

#include <cml/mathlib/mathlib.h>

#include <cassert>

using namespace Methane::Samples;

const AsteroidsApp::Vertex::FieldsArray AsteroidsApp::Vertex::layout;

// Common application settings
static const gfx::FrameSize           g_shadow_map_size(1024, 1024);
static const gfx::Shader::EntryTarget g_vs_main       = { "VSMain", "vs_5_1" };
static const gfx::Shader::EntryTarget g_ps_main       = { "PSMain", "ps_5_1" };
static const GraphicsApp::Settings    g_app_settings  = // Application settings:
{                                                       // ====================
    {                                                   // app:
        "Methane Asteroids",                            // - name
        0.8, 0.8,                                       // - width, height
    },                                                  //
    {                                                   // context:
        gfx::FrameSize(),                               // - frame_size
        gfx::PixelFormat::BGRA8Unorm,                   // - color_format
        gfx::PixelFormat::Depth32Float,                 // - depth_stencil_format
        gfx::Color(0.0f, 0.2f, 0.4f, 1.0f),             // - clear_color
        1.f,                                            // - clear_depth
        0,                                              // - clear_stencil
        3,                                              // - frame_buffers_count
        true,                                           // - vsync_enabled
    },                                                  //
    true                                                // show_hud_in_window_title
};

AsteroidsApp::AsteroidsApp()
    : GraphicsApp(g_app_settings, gfx::RenderPass::Access::ShaderResources | gfx::RenderPass::Access::Samplers)
    , m_cube_mesh(gfx::Mesh::VertexLayoutFromArray(Vertex::layout), 1.f, 1.f, 1.f)
    , m_floor_mesh(gfx::Mesh::VertexLayoutFromArray(Vertex::layout), 7.f, 7.f, 0.f, 0, gfx::RectMesh<Vertex>::FaceType::XZ)
    , m_scene_scale(15.f)
    , m_scene_constants(                                // Shader constants:
        {                                               // ================
            gfx::Color(1.f, 1.f, 0.74f, 1.f),           // - light_color
            600.f,                                      // - light_power
            0.2f,                                       // - light_ambient_factor
            5.f                                         // - light_specular_factor
        })
    , m_scene_camera(gfx::ArcBallCamera::Pivot::Aim)
    , m_light_camera(m_scene_camera, gfx::ArcBallCamera::Pivot::Aim)
{
    m_scene_camera.SetOrientation({ { 15.0f, 22.5f, -15.0f }, { 0.0f, 7.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } });
    m_light_camera.SetParamters({ 0.01f, 300.f, 90.f });
    m_scene_camera.SetZoomDistanceRange({ 15.f , 100.f });

    m_light_camera.SetOrientation({ { 0.0f,  25.0f, -25.0f }, { 0.0f, 7.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } });
    m_light_camera.SetProjection(gfx::Camera::Projection::Orthogonal);
    m_light_camera.SetParamters({ -300, 300.f, 90.f });
    m_light_camera.Resize(55, 55);

    m_input_state.SetControllers({
        std::make_shared<gfx::AppCameraController>(m_scene_camera),
        std::make_shared<gfx::AppCameraController>(m_light_camera,
            gfx::AppCameraController::MouseActionByButton {
                { Platform::Mouse::Button::Right, gfx::AppCameraController::MouseAction::Rotate },
            },
            gfx::AppCameraController::KeyboardActionByKey{ })
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
    const gfx::Context::Settings& context_settings = m_sp_context->GetSettings();

    // Create vertex and index buffer for meshes
    m_cube_buffers.Init(m_cube_mesh,   *m_sp_context, "Cube");
    m_floor_buffers.Init(m_floor_mesh, *m_sp_context, "Floor");
    m_scene_camera.Resize(static_cast<float>(context_settings.frame_size.width),
                          static_cast<float>(context_settings.frame_size.height));

    const Data::Size constants_data_size      = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(SceneUniforms)));
    const Data::Size scene_uniforms_data_size = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(SceneUniforms)));
    const Data::Size cube_uniforms_data_size  = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(MeshUniforms)));
    const Data::Size floor_uniforms_data_size = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(MeshUniforms)));

    // Create constants buffer for frame rendering
    m_sp_const_buffer = gfx::Buffer::CreateConstantBuffer(*m_sp_context, constants_data_size);
    m_sp_const_buffer->SetName("Constants Buffer");
    m_sp_const_buffer->SetData(reinterpret_cast<Data::ConstRawPtr>(&m_scene_constants), sizeof(m_scene_constants));

    // Load cube texture images from file
    m_sp_cube_texture = m_image_loader.CreateImageTexture(*m_sp_context, "Textures/MethaneBubbles.jpg");
    m_sp_cube_texture->SetName("Cube Texture Image");

    // Load floor texture images from file
    m_sp_floor_texture = m_image_loader.CreateImageTexture(*m_sp_context, "Textures/MarbleWhite.jpg");
    m_sp_floor_texture->SetName("Floor Texture Image");

    // Create sampler for image texture
    m_sp_texture_sampler = gfx::Sampler::Create(*m_sp_context, {
        { gfx::Sampler::Filter::MinMag::Linear     },    // Bilinear filtering
        { gfx::Sampler::Address::Mode::ClampToZero }
    });
    m_sp_texture_sampler->SetName("Texture Sampler");

    // Create sampler for shadow-map
    m_sp_shadow_sampler = gfx::Sampler::Create(*m_sp_context, {
        { gfx::Sampler::Filter::MinMag::Linear     },    // Bilinear filtering
        { gfx::Sampler::Address::Mode::ClampToEdge }
    });
    m_sp_shadow_sampler->SetName("Shadow Map Sampler");

    // ========= Final Pass objects =========

    // Create final-pass shading program with texturing
    gfx::Shader::MacroDefinitions textured_shadows_definitions = { { "ENABLE_SHADOWS", "" }, { "ENABLE_TEXTURING", "" } };
    m_final_pass.sp_program = gfx::Program::Create(*m_sp_context, {
        {
            gfx::Shader::CreateVertex(*m_sp_context, { g_vs_main, textured_shadows_definitions }),
            gfx::Shader::CreatePixel(*m_sp_context,  { g_ps_main, textured_shadows_definitions }),
        },
        { // input_buffer_layouts
            { // Signle vertex buffer with interleaved data:
                {
                    { "in_position", "POSITION" },
                    { "in_normal",   "NORMAL"   },
                    { "in_uv",       "TEXCOORD" },
                }
            }
        },
        { // constant_argument_names
            "g_constants", "g_texture_sampler", "g_shadow_sampler"
        },
        { // render_target_pixel_formats
            context_settings.color_format
        },
        context_settings.depth_stencil_format
    });
    m_final_pass.sp_program->SetName("Textured, Shadows & Lighting");

    // Create state for final pass rendering
    gfx::RenderState::Settings final_state_settings;
    final_state_settings.sp_program    = m_final_pass.sp_program;
    final_state_settings.viewports     = { gfx::GetFrameViewport(context_settings.frame_size) };
    final_state_settings.scissor_rects = { gfx::GetFrameScissorRect(context_settings.frame_size) };
    final_state_settings.depth.enabled = true;
    m_final_pass.sp_state = gfx::RenderState::Create(*m_sp_context, final_state_settings);
    m_final_pass.sp_state->SetName("Final pass render state");
    
    m_final_pass.command_group_name = "Final Render Pass";
    m_final_pass.is_final_pass = true;

    // ========= Shadow Pass objects =========
    
    // Shadow texture settings
    gfx::Texture::Settings shadow_texture_settings = gfx::Texture::Settings::DepthStencilBuffer(g_shadow_map_size, context_settings.depth_stencil_format, gfx::Texture::Usage::RenderTarget | gfx::Texture::Usage::ShaderRead);

    // Create shadow-pass program for geometry-only rendering to depth texture
    gfx::Shader::MacroDefinitions textured_definitions = { { "ENABLE_TEXTURING", "" } };
    m_shadow_pass.sp_program = gfx::Program::Create(*m_sp_context, {
        {
            gfx::Shader::CreateVertex(*m_sp_context, { g_vs_main, textured_definitions }),
        },
        m_final_pass.sp_program->GetSettings().input_buffer_layouts,
        {
            "g_constants", "g_shadow_sampler"
        },
        { // no color attachments, rendering to depth texture
        },
        shadow_texture_settings.pixel_format
    });
    m_shadow_pass.sp_program->SetName("Vertex Only: Textured, Lighting");

    // Create state for shadow map rendering
    gfx::RenderState::Settings shadow_state_settings;
    shadow_state_settings.sp_program    = m_shadow_pass.sp_program;
    shadow_state_settings.viewports     = { gfx::GetFrameViewport(g_shadow_map_size) };
    shadow_state_settings.scissor_rects = { gfx::GetFrameScissorRect(g_shadow_map_size) };
    shadow_state_settings.depth.enabled = true;
    m_shadow_pass.sp_state = gfx::RenderState::Create(*m_sp_context, shadow_state_settings);
    m_shadow_pass.sp_state->SetName("Shadow-map render state");
    
    m_shadow_pass.command_group_name = "Shadow Render Pass";
    m_shadow_pass.is_final_pass = false;

    // ========= Per-Frame Data =========
    for(AsteroidsFrame& frame : m_frames)
    {
        // Create uniforms buffer with volatile parameters for the whole scene rendering
        frame.sp_scene_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(*m_sp_context, scene_uniforms_data_size);
        frame.sp_scene_uniforms_buffer->SetName(IndexedName("Scene Uniforms Buffer", frame.index));

        // ========= Shadow Pass data =========

        // Create uniforms buffer for Cube rendering in Shadow pass
        frame.shadow_pass.cube.sp_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(*m_sp_context, cube_uniforms_data_size);
        frame.shadow_pass.cube.sp_uniforms_buffer->SetName(IndexedName("Cube Uniforms Buffer for Shadow Pass", frame.index));

        // Create uniforms buffer for Floor rendering in Shadow pass
        frame.shadow_pass.floor.sp_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(*m_sp_context, floor_uniforms_data_size);
        frame.shadow_pass.floor.sp_uniforms_buffer->SetName(IndexedName("Floor Uniforms Buffer for Shadow Pass", frame.index));

        // Create depth texture for shadow map rendering
        frame.shadow_pass.sp_rt_texture = gfx::Texture::CreateRenderTarget(*m_sp_context, shadow_texture_settings);
        frame.shadow_pass.sp_rt_texture->SetName(IndexedName("Shadow Map", frame.index));
        
        // Create shadow pass configuration with depth attachment
        frame.shadow_pass.sp_pass = gfx::RenderPass::Create(*m_sp_context, {
            { // No color attachments
            },
            gfx::RenderPass::DepthAttachment(
                {
                    frame.shadow_pass.sp_rt_texture,
                    0, 0, 0,
                    gfx::RenderPass::Attachment::LoadAction::Clear,
                    gfx::RenderPass::Attachment::StoreAction::Store,
                },
                context_settings.clear_depth
            ),
            gfx::RenderPass::StencilAttachment(),
            gfx::RenderPass::Access::ShaderResources
        });
        
        // Create render pass and command list for shadow pass rendering
        frame.shadow_pass.sp_cmd_list = gfx::RenderCommandList::Create(m_sp_context->GetRenderCommandQueue(), *frame.shadow_pass.sp_pass);
        frame.shadow_pass.sp_cmd_list->SetName(IndexedName("Shadow-Map Rendering", frame.index));

        // Shadow-pass resource bindings for cube rendering
        frame.shadow_pass.cube.sp_resource_bindings = gfx::Program::ResourceBindings::Create(m_shadow_pass.sp_program, {
            { { gfx::Shader::Type::All, "g_mesh_uniforms"  }, frame.shadow_pass.cube.sp_uniforms_buffer },
        });

        // Shadow-pass resource bindings for floor rendering
        frame.shadow_pass.floor.sp_resource_bindings = gfx::Program::ResourceBindings::Create(m_shadow_pass.sp_program, {
            { { gfx::Shader::Type::All, "g_mesh_uniforms"  }, frame.shadow_pass.floor.sp_uniforms_buffer },
        });

        // ========= Final Pass data =========

        // Create uniforms buffer for Cube rendering in Final pass
        frame.final_pass.cube.sp_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(*m_sp_context, cube_uniforms_data_size);
        frame.final_pass.cube.sp_uniforms_buffer->SetName(IndexedName("Cube Uniforms Buffer for Final Pass", frame.index));

        // Create uniforms buffer for Floor rendering in Final pass
        frame.final_pass.floor.sp_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(*m_sp_context, floor_uniforms_data_size);
        frame.final_pass.floor.sp_uniforms_buffer->SetName(IndexedName("Floor Uniforms Buffer for Final Pass", frame.index));

        // Bind final pass RT texture and pass to the frame buffer texture and final pass.
        frame.final_pass.sp_rt_texture = frame.sp_screen_texture;
        frame.final_pass.sp_pass       = frame.sp_screen_pass;
        
        // Create render pass and command list for final pass rendering
        frame.final_pass.sp_cmd_list = gfx::RenderCommandList::Create(m_sp_context->GetRenderCommandQueue(), *frame.final_pass.sp_pass);
        frame.final_pass.sp_cmd_list->SetName(IndexedName("Final Scene Rendering", frame.index));

        // Final-pass resource bindings for cube rendering
        frame.final_pass.cube.sp_resource_bindings = gfx::Program::ResourceBindings::Create(m_final_pass.sp_program, {
            { { gfx::Shader::Type::Vertex, "g_mesh_uniforms"  }, frame.final_pass.cube.sp_uniforms_buffer    },
            { { gfx::Shader::Type::Pixel,  "g_scene_uniforms" }, frame.sp_scene_uniforms_buffer              },
            { { gfx::Shader::Type::Pixel,  "g_constants"      }, m_sp_const_buffer                           },
            { { gfx::Shader::Type::Pixel,  "g_shadow_map"     }, frame.shadow_pass.sp_rt_texture             },
            { { gfx::Shader::Type::Pixel,  "g_shadow_sampler" }, m_sp_shadow_sampler                         },
            { { gfx::Shader::Type::Pixel,  "g_texture"        }, m_sp_cube_texture                           },
            { { gfx::Shader::Type::Pixel,  "g_texture_sampler"}, m_sp_texture_sampler                        },
        });

        // Final-pass resource bindings for floor rendering - patched a copy of cube bindings
        frame.final_pass.floor.sp_resource_bindings = gfx::Program::ResourceBindings::CreateCopy(*frame.final_pass.cube.sp_resource_bindings, {
            { { gfx::Shader::Type::Vertex, "g_mesh_uniforms"  }, frame.final_pass.floor.sp_uniforms_buffer   },
            { { gfx::Shader::Type::Pixel,  "g_texture"        }, m_sp_floor_texture                          },
        });
    }

    // Complete initialization of render context:
    //  - allocate deferred descriptor heaps with calculated sizes
    //  - execute commands to upload resources to GPU
    m_sp_context->CompleteInitialization();
}

template<typename VType>
void AsteroidsApp::MeshBuffers::Init(const gfx::BaseMesh<VType>& mesh_data, gfx::Context& context, const std::string& base_name)
{
    // Create vertex buffer of the mesh
    const Data::Size vertex_data_size = static_cast<Data::Size>(mesh_data.GetVertexDataSize());
    const Data::Size vertex_size = static_cast<Data::Size>(mesh_data.GetVertexSize());
    sp_vertex = gfx::Buffer::CreateVertexBuffer(context, vertex_data_size, vertex_size);
    sp_vertex->SetName(base_name + " Vertex Buffer");
    sp_vertex->SetData(reinterpret_cast<Data::ConstRawPtr>(mesh_data.GetVertices().data()), vertex_data_size);

    // Create index buffer of the mesh
    const Data::Size floor_index_data_size = static_cast<Data::Size>(mesh_data.GetIndexDataSize());
    sp_index = gfx::Buffer::CreateIndexBuffer(context, floor_index_data_size, gfx::PixelFormat::R32Uint);
    sp_index->SetName(base_name + " Index Buffer");
    sp_index->SetData(reinterpret_cast<Data::ConstRawPtr>(mesh_data.GetIndices().data()), floor_index_data_size);
}

bool AsteroidsApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    if (!m_initialized || m_context_settings.frame_size == frame_size)
        return false;

    // Resize screen color and depth textures
    for (AsteroidsFrame& frame : m_frames)
        frame.final_pass.sp_rt_texture.reset();

    GraphicsApp::Resize(frame_size, is_minimized);

    for (AsteroidsFrame& frame : m_frames)
        frame.final_pass.sp_rt_texture = frame.sp_screen_texture;

    // Update viewports and scissor rects state
    assert(m_final_pass.sp_state);
    m_final_pass.sp_state->SetViewports({ gfx::GetFrameViewport(frame_size) });
    m_final_pass.sp_state->SetScissorRects({ gfx::GetFrameScissorRect(frame_size) });

    m_scene_camera.Resize(static_cast<float>(frame_size.width), static_cast<float>(frame_size.height));
    return true;
}

void AsteroidsApp::Update()
{
    if (HasError())
        return;

    // Update Model, View, Projection matrices based on scene camera location
    gfx::Matrix44f scale_matrix, scene_view_matrix, scene_proj_matrix;
    cml::matrix_uniform_scale(scale_matrix, m_scene_scale);
    m_scene_camera.GetViewProjMatrices(scene_view_matrix, scene_proj_matrix);

    // Update View and Projection matrices based on light camera location
    gfx::Matrix44f light_view_matrix, light_proj_matrix;
    m_light_camera.GetViewProjMatrices(light_view_matrix, light_proj_matrix);
    
    // Prepare shadow transform matrix
    static const gfx::Matrix44f shadow_transform_matrix = ([]() -> gfx::Matrix44f
    {
        gfx::Matrix44f shadow_scale_matrix, shadow_translate_matrix;
        cml::matrix_scale(shadow_scale_matrix, 0.5f, -0.5f, 1.f);
        cml::matrix_translation(shadow_translate_matrix, 0.5f, 0.5f, 0.f);
        return shadow_scale_matrix * shadow_translate_matrix;
    })();

    // Update per-frame uniform buffers
    AsteroidsFrame& frame = GetCurrentFrame();
    assert(!!frame.sp_scene_uniforms_buffer);

    // Update scene uniforms
    m_scene_uniforms.eye_position    = gfx::Vector4f(m_scene_camera.GetOrientation().eye, 1.f);
    m_scene_uniforms.light_position  = m_light_camera.GetOrientation().eye;

    frame.sp_scene_uniforms_buffer->SetData(reinterpret_cast<Data::ConstRawPtr>(&m_scene_uniforms), sizeof(m_scene_uniforms));

    // Cube model matrix
    gfx::Matrix44f cube_model_matrix;
    cml::matrix_translation(cube_model_matrix, gfx::Vector3f(0.f, m_cube_mesh.GetHeight() / 2.f, 0.f));
    cube_model_matrix = cube_model_matrix * scale_matrix;

    // Update Cube uniforms with matrices for Final pass
    {
        MeshUniforms mesh_uniforms          = {};
        mesh_uniforms.model_matrix          = cube_model_matrix;
        mesh_uniforms.mvp_matrix            = mesh_uniforms.model_matrix * scene_view_matrix * scene_proj_matrix;
        mesh_uniforms.shadow_mvpx_matrix    = mesh_uniforms.model_matrix * light_view_matrix * light_proj_matrix * shadow_transform_matrix;

        frame.final_pass.cube.sp_uniforms_buffer->SetData(reinterpret_cast<Data::ConstRawPtr>(&mesh_uniforms), sizeof(mesh_uniforms));
    }
    // Update Cube uniforms with matrices for Shadow pass
    {
        MeshUniforms mesh_uniforms          = {};
        mesh_uniforms.model_matrix          = cube_model_matrix;
        mesh_uniforms.mvp_matrix            = mesh_uniforms.model_matrix * light_view_matrix * light_proj_matrix;

        frame.shadow_pass.cube.sp_uniforms_buffer->SetData(reinterpret_cast<Data::ConstRawPtr>(&mesh_uniforms), sizeof(mesh_uniforms));
    }
    // Update Floor uniforms with matrices for Final pass
    {
        MeshUniforms mesh_uniforms          = {};
        mesh_uniforms.model_matrix          = scale_matrix;
        mesh_uniforms.mvp_matrix            = mesh_uniforms.model_matrix * scene_view_matrix * scene_proj_matrix;
        mesh_uniforms.shadow_mvpx_matrix    = mesh_uniforms.model_matrix * light_view_matrix * light_proj_matrix * shadow_transform_matrix;

        frame.final_pass.floor.sp_uniforms_buffer->SetData(reinterpret_cast<Data::ConstRawPtr>(&mesh_uniforms), sizeof(mesh_uniforms));
    }
    // Update Floor uniforms with matrices for Shadow pass
    {
        MeshUniforms mesh_uniforms          = {};
        mesh_uniforms.model_matrix          = scale_matrix;
        mesh_uniforms.mvp_matrix            = mesh_uniforms.model_matrix * light_view_matrix * light_proj_matrix;

        frame.shadow_pass.floor.sp_uniforms_buffer->SetData(reinterpret_cast<Data::ConstRawPtr>(&mesh_uniforms), sizeof(mesh_uniforms));
    }
}

void AsteroidsApp::Render()
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

    // Record commands for shadow & final render passes
    AsteroidsFrame& frame = GetCurrentFrame();
    RenderScene(m_shadow_pass, frame.shadow_pass, *frame.shadow_pass.sp_rt_texture, true);
    RenderScene(m_final_pass,  frame.final_pass,  *frame.shadow_pass.sp_rt_texture, false);

    // Execute rendering commands and present frame to screen
    m_sp_context->GetRenderCommandQueue().Execute({
        *frame.shadow_pass.sp_cmd_list,
        *frame.final_pass.sp_cmd_list
    });
    m_sp_context->Present();

    GraphicsApp::Render();
}

void AsteroidsApp::RenderScene(const RenderPass& render_pass, AsteroidsFrame::PassResources& render_pass_resources, gfx::Texture& shadow_texture, bool is_shadow_rendering)
{
    assert(!!render_pass_resources.sp_cmd_list);
    gfx::RenderCommandList& cmd_list = *render_pass_resources.sp_cmd_list;

    assert(!!render_pass.sp_state);
    cmd_list.Reset(*render_pass.sp_state, render_pass.command_group_name);

    // Cube drawing

    assert(!!render_pass_resources.cube.sp_resource_bindings);
    assert(!!m_cube_buffers.sp_vertex);
    assert(!!m_cube_buffers.sp_index);

    cmd_list.SetResourceBindings(*render_pass_resources.cube.sp_resource_bindings);
    cmd_list.SetVertexBuffers({ *m_cube_buffers.sp_vertex });
    cmd_list.DrawIndexed(gfx::RenderCommandList::Primitive::Triangle, *m_cube_buffers.sp_index, 1);

    // Floor drawing

    assert(!!render_pass_resources.floor.sp_resource_bindings);
    assert(!!m_floor_buffers.sp_vertex);
    assert(!!m_floor_buffers.sp_index);

    cmd_list.SetResourceBindings(*render_pass_resources.floor.sp_resource_bindings);
    cmd_list.SetVertexBuffers({ *m_floor_buffers.sp_vertex });
    cmd_list.DrawIndexed(gfx::RenderCommandList::Primitive::Triangle, *m_floor_buffers.sp_index, 1);

    cmd_list.Commit(render_pass.is_final_pass);
}

int main(int argc, const char* argv[])
{
    return AsteroidsApp().Run({ argc, argv });
}
