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

FILE: ShadowCubeApp.cpp
Tutorial demonstrating shadow-pass rendering with Methane graphics API

******************************************************************************/

#include "ShadowCubeApp.h"

#include <Methane/Data/TimeAnimation.h>

#include <cml/mathlib/mathlib.h>
#include <cassert>

namespace Methane::Tutorials
{

// Common application settings
static const gfx::FrameSize           g_shadow_map_size(1024, 1024);
static const gfx::Shader::EntryTarget g_vs_main       = { "Shaders", "VSMain", "vs_5_1" };
static const gfx::Shader::EntryTarget g_ps_main       = { "Shaders", "PSMain", "ps_5_1" };
static const GraphicsApp::Settings    g_app_settings  = // Application settings:
{                                                       // ====================
    {                                                   // app:
        "Methane Shadow Cube",                          // - name
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

ShadowCubeApp::ShadowCubeApp()
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
{
    m_view_camera.SetOrientation({ { 15.0f, 22.5f, -15.0f }, { 0.0f, 7.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } });

    m_light_camera.SetOrientation({ { 0.0f,  25.0f, -25.0f }, { 0.0f, 7.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } });
    m_light_camera.SetProjection(gfx::Camera::Projection::Orthogonal);
    m_light_camera.SetParamters({ -300, 300.f, 90.f });
    m_light_camera.Resize(80, 80);

    m_animations.push_back(
        std::make_shared<Data::TimeAnimation>(
            [this](double, double delta_seconds)
            {
                m_view_camera.RotateYaw(static_cast<float>(delta_seconds * 360.f / 8.f));
                m_light_camera.RotateYaw(static_cast<float>(delta_seconds * 360.f / 4.f));
                return true;
            }));
}

ShadowCubeApp::~ShadowCubeApp()
{
    // Wait for GPU rendering is completed to release resources
    m_sp_context->WaitForGpu(gfx::Context::WaitFor::RenderComplete);
}

void ShadowCubeApp::Init()
{
    GraphicsApp::Init();

    assert(m_sp_context);
    const gfx::Context::Settings& context_settings = m_sp_context->GetSettings();

    // Load textures, vertex and index buffers for cube and floor meshes
    m_sp_cube_buffers  = std::make_unique<TexturedMeshBuffers>(*m_sp_context, m_cube_mesh, m_image_loader,
                                                               "Textures/MethaneBubbles.jpg", "Cube");
    m_sp_floor_buffers = std::make_unique<TexturedMeshBuffers>(*m_sp_context, m_floor_mesh, m_image_loader,
                                                               "Textures/MarbleWhite.jpg", "Floor");

    m_view_camera.Resize(static_cast<float>(context_settings.frame_size.width),
                         static_cast<float>(context_settings.frame_size.height));

    const Data::Size constants_data_size      = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(Constants)));
    const Data::Size scene_uniforms_data_size = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(SceneUniforms)));
    const Data::Size cube_uniforms_data_size  = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(MeshUniforms)));
    const Data::Size floor_uniforms_data_size = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(MeshUniforms)));

    // Create constants buffer for frame rendering
    m_sp_const_buffer = gfx::Buffer::CreateConstantBuffer(*m_sp_context, constants_data_size);
    m_sp_const_buffer->SetName("Constants Buffer");
    m_sp_const_buffer->SetData(reinterpret_cast<Data::ConstRawPtr>(&m_scene_constants), sizeof(m_scene_constants));

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
    for(ShadowCubeFrame& frame : m_frames)
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
            { { gfx::Shader::Type::Pixel,  "g_texture"        }, m_sp_cube_buffers->sp_texture               },
            { { gfx::Shader::Type::Pixel,  "g_texture_sampler"}, m_sp_texture_sampler                        },
        });

        // Final-pass resource bindings for floor rendering - patched a copy of cube bindings
        frame.final_pass.floor.sp_resource_bindings = gfx::Program::ResourceBindings::CreateCopy(*frame.final_pass.cube.sp_resource_bindings, {
            { { gfx::Shader::Type::Vertex, "g_mesh_uniforms"  }, frame.final_pass.floor.sp_uniforms_buffer   },
            { { gfx::Shader::Type::Pixel,  "g_texture"        }, m_sp_floor_buffers->sp_texture              },
        });
    }

    // Complete initialization of render context:
    //  - allocate deferred descriptor heaps with calculated sizes
    //  - execute commands to upload resources to GPU
    m_sp_context->CompleteInitialization();
}

void ShadowCubeApp::RenderPass::Release()
{
    sp_program.reset();
    sp_state.reset();
}

bool ShadowCubeApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    if (!m_initialized || GetInitialContextSettings().frame_size == frame_size)
        return false;

    // Resize screen color and depth textures
    for (ShadowCubeFrame& frame : m_frames)
        frame.final_pass.sp_rt_texture.reset();

    GraphicsApp::Resize(frame_size, is_minimized);

    for (ShadowCubeFrame& frame : m_frames)
        frame.final_pass.sp_rt_texture = frame.sp_screen_texture;

    // Update viewports and scissor rects state
    assert(m_final_pass.sp_state);
    m_final_pass.sp_state->SetViewports({ gfx::GetFrameViewport(frame_size) });
    m_final_pass.sp_state->SetScissorRects({ gfx::GetFrameScissorRect(frame_size) });

    m_view_camera.Resize(static_cast<float>(frame_size.width), static_cast<float>(frame_size.height));
    return true;
}

void ShadowCubeApp::Update()
{
    GraphicsApp::Update();

    // Update Model, View, Projection matrices based on scene camera location
    gfx::Matrix44f scale_matrix, scene_view_matrix, scene_proj_matrix;
    cml::matrix_uniform_scale(scale_matrix, m_scene_scale);
    m_view_camera.GetViewProjMatrices(scene_view_matrix, scene_proj_matrix);

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

    // Update scene uniforms
    m_scene_uniforms.eye_position    = gfx::Vector4f(m_view_camera.GetOrientation().eye, 1.f);
    m_scene_uniforms.light_position  = m_light_camera.GetOrientation().eye;

    // Cube model matrix
    gfx::Matrix44f cube_model_matrix;
    cml::matrix_translation(cube_model_matrix, gfx::Vector3f(0.f, m_cube_mesh.GetHeight() / 2.f, 0.f));
    cube_model_matrix = cube_model_matrix * scale_matrix;

    // Update Cube uniforms with matrices for Final pass
    {
        MeshUniforms& mesh_uniforms         = m_sp_cube_buffers->final_pass_uniforms;
        mesh_uniforms.model_matrix          = cube_model_matrix;
        mesh_uniforms.mvp_matrix            = mesh_uniforms.model_matrix * scene_view_matrix * scene_proj_matrix;
        mesh_uniforms.shadow_mvpx_matrix    = mesh_uniforms.model_matrix * light_view_matrix * light_proj_matrix * shadow_transform_matrix;
    }
    // Update Cube uniforms with matrices for Shadow pass
    {
        MeshUniforms& mesh_uniforms         = m_sp_cube_buffers->shadow_pass_uniforms;
        mesh_uniforms.model_matrix          = cube_model_matrix;
        mesh_uniforms.mvp_matrix            = mesh_uniforms.model_matrix * light_view_matrix * light_proj_matrix;
    }
    // Update Floor uniforms with matrices for Final pass
    {
        MeshUniforms& mesh_uniforms         = m_sp_floor_buffers->final_pass_uniforms;
        mesh_uniforms.model_matrix          = scale_matrix;
        mesh_uniforms.mvp_matrix            = mesh_uniforms.model_matrix * scene_view_matrix * scene_proj_matrix;
        mesh_uniforms.shadow_mvpx_matrix    = mesh_uniforms.model_matrix * light_view_matrix * light_proj_matrix * shadow_transform_matrix;
    }
    // Update Floor uniforms with matrices for Shadow pass
    {
        MeshUniforms& mesh_uniforms         = m_sp_floor_buffers->shadow_pass_uniforms;
        mesh_uniforms.model_matrix          = scale_matrix;
        mesh_uniforms.mvp_matrix            = mesh_uniforms.model_matrix * light_view_matrix * light_proj_matrix;
    }
}

void ShadowCubeApp::Render()
{
    // Render only when context is ready
    assert(!!m_sp_context);
    if (!m_sp_context->ReadyToRender())
        return;

    // Wait for previous frame rendering is completed and switch to next frame
    m_sp_context->WaitForGpu(gfx::Context::WaitFor::FramePresented);
    ShadowCubeFrame& frame = GetCurrentFrame();

    // Upload uniform buffers to GPU
    frame.sp_scene_uniforms_buffer->SetData(reinterpret_cast<Data::ConstRawPtr>(&m_scene_uniforms), sizeof(SceneUniforms));
    frame.shadow_pass.floor.sp_uniforms_buffer->SetData(reinterpret_cast<Data::ConstRawPtr>(&m_sp_floor_buffers->shadow_pass_uniforms), sizeof(MeshUniforms));
    frame.shadow_pass.cube.sp_uniforms_buffer->SetData(reinterpret_cast<Data::ConstRawPtr>(&m_sp_cube_buffers->shadow_pass_uniforms), sizeof(MeshUniforms));
    frame.final_pass.floor.sp_uniforms_buffer->SetData(reinterpret_cast<Data::ConstRawPtr>(&m_sp_floor_buffers->final_pass_uniforms), sizeof(MeshUniforms));
    frame.final_pass.cube.sp_uniforms_buffer->SetData(reinterpret_cast<Data::ConstRawPtr>(&m_sp_cube_buffers->final_pass_uniforms), sizeof(MeshUniforms));

    // Record commands for shadow & final render passes
    RenderScene(m_shadow_pass, frame.shadow_pass, *frame.shadow_pass.sp_rt_texture);
    RenderScene(m_final_pass, frame.final_pass, *frame.shadow_pass.sp_rt_texture);

    // Execute rendering commands and present frame to screen
    m_sp_context->GetRenderCommandQueue().Execute({
        *frame.shadow_pass.sp_cmd_list,
        *frame.final_pass.sp_cmd_list
    });
    m_sp_context->Present();

    GraphicsApp::Render();
}

void ShadowCubeApp::RenderScene(const RenderPass &render_pass, ShadowCubeFrame::PassResources &render_pass_resources, gfx::Texture &shadow_texture)
{
    assert(!!render_pass_resources.sp_cmd_list);
    gfx::RenderCommandList& cmd_list = *render_pass_resources.sp_cmd_list;

    assert(!!render_pass.sp_state);
    cmd_list.Reset(*render_pass.sp_state, render_pass.command_group_name);

    // Cube drawing
    assert(!!render_pass_resources.cube.sp_resource_bindings);
    assert(!!m_sp_cube_buffers);
    m_sp_cube_buffers->Draw(cmd_list, *render_pass_resources.cube.sp_resource_bindings, 1);

    // Floor drawing
    assert(!!render_pass_resources.floor.sp_resource_bindings);
    assert(!!m_sp_floor_buffers);
    m_sp_floor_buffers->Draw(cmd_list, *render_pass_resources.floor.sp_resource_bindings, 1);

    cmd_list.Commit(render_pass.is_final_pass);
}

void ShadowCubeApp::OnContextReleased()
{
    m_final_pass.Release();
    m_shadow_pass.Release();

    m_sp_floor_buffers.reset();
    m_sp_cube_buffers.reset();
    m_sp_shadow_sampler.reset();
    m_sp_texture_sampler.reset();
    m_sp_const_buffer.reset();

    GraphicsApp::OnContextReleased();
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::ShadowCubeApp().Run({ argc, argv });
}
