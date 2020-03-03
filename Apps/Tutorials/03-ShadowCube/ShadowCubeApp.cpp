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

FILE: ShadowCubeApp.cpp
Tutorial demonstrating shadow-pass rendering with Methane graphics API

******************************************************************************/

#include "ShadowCubeApp.h"

#include <Methane/Graphics/Mesh/CubeMesh.hpp>
#include <Methane/Data/TimeAnimation.h>

#include <cml/mathlib/mathlib.h>

namespace Methane::Tutorials
{

struct Vertex
{
    gfx::Mesh::Position position;
    gfx::Mesh::Normal   normal;
    gfx::Mesh::TexCoord texcoord;

    inline static const gfx::Mesh::VertexLayout layout = {
        gfx::Mesh::VertexField::Position,
        gfx::Mesh::VertexField::Normal,
        gfx::Mesh::VertexField::TexCoord,
    };
};

// Common application settings
static const gfx::FrameSize           g_shadow_map_size(1024, 1024);
static const GraphicsApp::AllSettings g_app_settings =  // Application settings:
{                                                       // ====================
    {                                                   // platform_app:
        "Methane Shadow Cube",                          // - name
        0.8, 0.8,                                       // - width, height
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
        gfx::Color4f(0.0f, 0.2f, 0.4f, 1.0f),           // - clear_color
        gfx::DepthStencil{ 1.f, 0 },                    // - clear_depth_stencil
        3,                                              // - frame_buffers_count
        false,                                          // - vsync_enabled
    }
};

ShadowCubeApp::ShadowCubeApp()
    : GraphicsApp(g_app_settings, "Methane tutorial of shadow pass rendering")
    , m_scene_scale(15.f)
    , m_scene_constants(                                // Shader constants:
        {                                               // ================
            gfx::Color4f(1.f, 1.f, 0.74f, 1.f),         // - light_color
            600.f,                                      // - light_power
            0.2f,                                       // - light_ambient_factor
            5.f                                         // - light_specular_factor
        })
    , m_shadow_pass(false, "Shadow Render Pass")
    , m_final_pass(true, "Final Render Pass")
{
    m_view_camera.SetOrientation({ { 15.0f, 22.5f, -15.0f }, { 0.0f, 7.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } });

    m_light_camera.SetOrientation({ { 0.0f,  25.0f, -25.0f }, { 0.0f, 7.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } });
    m_light_camera.SetProjection(gfx::Camera::Projection::Orthogonal);
    m_light_camera.SetParameters({ -300, 300.f, 90.f });
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

    const gfx::RenderContext::Settings& context_settings = m_sp_context->GetSettings();
    m_view_camera.Resize(static_cast<float>(context_settings.frame_size.width),
                         static_cast<float>(context_settings.frame_size.height));

    const gfx::Mesh::VertexLayout mesh_layout(Vertex::layout);
    const gfx::CubeMesh<Vertex>  cube_mesh(mesh_layout, 1.f, 1.f, 1.f);
    const gfx::QuadMesh<Vertex> floor_mesh(mesh_layout, 7.f, 7.f, 0.f, 0, gfx::QuadMesh<Vertex>::FaceType::XZ);

    // Load textures, vertex and index buffers for cube and floor meshes
    m_sp_cube_buffers  = std::make_unique<TexturedMeshBuffers>(*m_sp_context, cube_mesh, "Cube");
    m_sp_cube_buffers->SetTexture(m_image_loader.LoadImageToTexture2D(*m_sp_context, "Textures/MethaneBubbles.jpg", true));

    m_sp_floor_buffers = std::make_unique<TexturedMeshBuffers>(*m_sp_context, floor_mesh, "Floor");
    m_sp_floor_buffers->SetTexture(m_image_loader.LoadImageToTexture2D(*m_sp_context, "Textures/MarbleWhite.jpg", true));

    const Data::Size constants_data_size      = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(Constants)));
    const Data::Size scene_uniforms_data_size = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(SceneUniforms)));
    const Data::Size mesh_uniforms_data_size  = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(MeshUniforms)));

    // Create constants buffer for frame rendering
    m_sp_const_buffer = gfx::Buffer::CreateConstantBuffer(*m_sp_context, constants_data_size);
    m_sp_const_buffer->SetName("Constants Buffer");
    m_sp_const_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&m_scene_constants), sizeof(m_scene_constants) } });

    // Create sampler for cube and floor textures sampling
    m_sp_texture_sampler = gfx::Sampler::Create(*m_sp_context,
        gfx::Sampler::Settings
        {
            gfx::Sampler::Filter  { gfx::Sampler::Filter::MinMag::Linear },
            gfx::Sampler::Address { gfx::Sampler::Address::Mode::ClampToEdge }
        }
    );
    m_sp_texture_sampler->SetName("Texture Sampler");

    // Create sampler for shadow-map texture
    m_sp_shadow_sampler = gfx::Sampler::Create(*m_sp_context,
        gfx::Sampler::Settings
        {
            gfx::Sampler::Filter  { gfx::Sampler::Filter::MinMag::Linear },
            gfx::Sampler::Address { gfx::Sampler::Address::Mode::ClampToEdge }
        }
    );
    m_sp_shadow_sampler->SetName("Shadow Map Sampler");

    // ========= Final Pass objects =========

    const gfx::Shader::EntryFunction    vs_main                      = { "ShadowCube", "CubeVS" };
    const gfx::Shader::EntryFunction    ps_main                      = { "ShadowCube", "CubePS" };
    const gfx::Shader::MacroDefinitions textured_shadows_definitions = { { "ENABLE_SHADOWS", "" }, { "ENABLE_TEXTURING", "" } };

    // Create final pass rendering state with program
    gfx::RenderState::Settings final_state_settings;
    final_state_settings.sp_program = gfx::Program::Create(*m_sp_context,
        gfx::Program::Settings
        {
            gfx::Program::Shaders
            {
                gfx::Shader::CreateVertex(*m_sp_context, { Data::ShaderProvider::Get(), vs_main, textured_shadows_definitions }),
                gfx::Shader::CreatePixel(*m_sp_context,  { Data::ShaderProvider::Get(), ps_main, textured_shadows_definitions }),
            },
            gfx::Program::InputBufferLayouts
            {
                gfx::Program::InputBufferLayout
                {
                    gfx::Program::InputBufferLayout::ArgumentSemantics { cube_mesh.GetVertexLayout().GetSemantics() }
                }
            },
            gfx::Program::ArgumentDescriptions
            {
                { { gfx::Shader::Type::Vertex, "g_mesh_uniforms"  }, gfx::Program::Argument::Modifiers::None     },
                { { gfx::Shader::Type::Pixel,  "g_scene_uniforms" }, gfx::Program::Argument::Modifiers::None     },
                { { gfx::Shader::Type::Pixel,  "g_constants"      }, gfx::Program::Argument::Modifiers::Constant },
                { { gfx::Shader::Type::Pixel,  "g_shadow_map"     }, gfx::Program::Argument::Modifiers::None     },
                { { gfx::Shader::Type::Pixel,  "g_shadow_sampler" }, gfx::Program::Argument::Modifiers::Constant },
                { { gfx::Shader::Type::Pixel,  "g_texture"        }, gfx::Program::Argument::Modifiers::None     },
                { { gfx::Shader::Type::Pixel,  "g_texture_sampler"}, gfx::Program::Argument::Modifiers::Constant },
            },
            gfx::PixelFormats
            {
                context_settings.color_format
            },
            context_settings.depth_stencil_format
        }
    );
    final_state_settings.sp_program->SetName("Textured, Shadows & Lighting");
    final_state_settings.viewports     = { gfx::GetFrameViewport(context_settings.frame_size) };
    final_state_settings.scissor_rects = { gfx::GetFrameScissorRect(context_settings.frame_size) };
    final_state_settings.depth.enabled = true;
    m_final_pass.sp_state = gfx::RenderState::Create(*m_sp_context, final_state_settings);
    m_final_pass.sp_state->SetName("Final pass render state");

    // ========= Shadow Pass objects =========
    
    gfx::Texture::Settings        shadow_texture_settings = gfx::Texture::Settings::DepthStencilBuffer(g_shadow_map_size, context_settings.depth_stencil_format, gfx::Texture::Usage::RenderTarget | gfx::Texture::Usage::ShaderRead);
    gfx::Shader::MacroDefinitions textured_definitions    = { { "ENABLE_TEXTURING", "" } };

    // Create shadow-pass rendering state with program
    gfx::RenderState::Settings shadow_state_settings;
    shadow_state_settings.sp_program = gfx::Program::Create(*m_sp_context,
        gfx::Program::Settings
        {
            gfx::Program::Shaders
            {
                gfx::Shader::CreateVertex(*m_sp_context, { Data::ShaderProvider::Get(), vs_main, textured_definitions }),
            },
            final_state_settings.sp_program->GetSettings().input_buffer_layouts,
            gfx::Program::ArgumentDescriptions
            {
                { { gfx::Shader::Type::All, "g_mesh_uniforms"  }, gfx::Program::Argument::Modifiers::None },
            },
            gfx::PixelFormats { /* no color attachments, rendering to depth texture */ },
            shadow_texture_settings.pixel_format
        }
    );
    shadow_state_settings.sp_program->SetName("Vertex Only: Textured, Lighting");
    shadow_state_settings.viewports     = { gfx::GetFrameViewport(g_shadow_map_size) };
    shadow_state_settings.scissor_rects = { gfx::GetFrameScissorRect(g_shadow_map_size) };
    shadow_state_settings.depth.enabled = true;
    m_shadow_pass.sp_state = gfx::RenderState::Create(*m_sp_context, shadow_state_settings);
    m_shadow_pass.sp_state->SetName("Shadow-map render state");

    // ========= Per-Frame Data =========
    for(ShadowCubeFrame& frame : m_frames)
    {
        // Create uniforms buffer with volatile parameters for the whole scene rendering
        frame.sp_scene_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(*m_sp_context, scene_uniforms_data_size);
        frame.sp_scene_uniforms_buffer->SetName(IndexedName("Scene Uniforms Buffer", frame.index));

        // ========= Shadow Pass data =========

        // Create uniforms buffer for Cube rendering in Shadow pass
        frame.shadow_pass.cube.sp_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(*m_sp_context, mesh_uniforms_data_size);
        frame.shadow_pass.cube.sp_uniforms_buffer->SetName(IndexedName("Cube Uniforms Buffer for Shadow Pass", frame.index));

        // Create uniforms buffer for Floor rendering in Shadow pass
        frame.shadow_pass.floor.sp_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(*m_sp_context, mesh_uniforms_data_size);
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
                context_settings.clear_depth_stencil->first
            ),
            gfx::RenderPass::StencilAttachment(),
            gfx::RenderPass::Access::ShaderResources
        });
        
        // Create render pass and command list for shadow pass rendering
        frame.shadow_pass.sp_cmd_list = gfx::RenderCommandList::Create(m_sp_context->GetRenderCommandQueue(), *frame.shadow_pass.sp_pass);
        frame.shadow_pass.sp_cmd_list->SetName(IndexedName("Shadow-Map Rendering", frame.index));

        // Shadow-pass resource bindings for cube rendering
        frame.shadow_pass.cube.sp_program_bindings = gfx::ProgramBindings::Create(shadow_state_settings.sp_program, {
            { { gfx::Shader::Type::All, "g_mesh_uniforms"  }, { { frame.shadow_pass.cube.sp_uniforms_buffer } } },
        });

        // Shadow-pass resource bindings for floor rendering
        frame.shadow_pass.floor.sp_program_bindings = gfx::ProgramBindings::Create(shadow_state_settings.sp_program, {
            { { gfx::Shader::Type::All, "g_mesh_uniforms"  }, { { frame.shadow_pass.floor.sp_uniforms_buffer } } },
        });

        // ========= Final Pass data =========

        // Create uniforms buffer for Cube rendering in Final pass
        frame.final_pass.cube.sp_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(*m_sp_context, mesh_uniforms_data_size);
        frame.final_pass.cube.sp_uniforms_buffer->SetName(IndexedName("Cube Uniforms Buffer for Final Pass", frame.index));

        // Create uniforms buffer for Floor rendering in Final pass
        frame.final_pass.floor.sp_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(*m_sp_context, mesh_uniforms_data_size);
        frame.final_pass.floor.sp_uniforms_buffer->SetName(IndexedName("Floor Uniforms Buffer for Final Pass", frame.index));

        // Bind final pass RT texture and pass to the frame buffer texture and final pass.
        frame.final_pass.sp_rt_texture = frame.sp_screen_texture;
        frame.final_pass.sp_pass       = frame.sp_screen_pass;
        
        // Create render pass and command list for final pass rendering
        frame.final_pass.sp_cmd_list = gfx::RenderCommandList::Create(m_sp_context->GetRenderCommandQueue(), *frame.final_pass.sp_pass);
        frame.final_pass.sp_cmd_list->SetName(IndexedName("Final Scene Rendering", frame.index));

        // Final-pass resource bindings for cube rendering
        frame.final_pass.cube.sp_program_bindings = gfx::ProgramBindings::Create(final_state_settings.sp_program, {
            { { gfx::Shader::Type::Vertex, "g_mesh_uniforms"  }, { { frame.final_pass.cube.sp_uniforms_buffer   } } },
            { { gfx::Shader::Type::Pixel,  "g_scene_uniforms" }, { { frame.sp_scene_uniforms_buffer             } } },
            { { gfx::Shader::Type::Pixel,  "g_constants"      }, { { m_sp_const_buffer                          } } },
            { { gfx::Shader::Type::Pixel,  "g_shadow_map"     }, { { frame.shadow_pass.sp_rt_texture            } } },
            { { gfx::Shader::Type::Pixel,  "g_shadow_sampler" }, { { m_sp_shadow_sampler                        } } },
            { { gfx::Shader::Type::Pixel,  "g_texture"        }, { { m_sp_cube_buffers->GetTexturePtr()         } } },
            { { gfx::Shader::Type::Pixel,  "g_texture_sampler"}, { { m_sp_texture_sampler                       } } },
        });

        // Final-pass resource bindings for floor rendering - patched a copy of cube bindings
        frame.final_pass.floor.sp_program_bindings = gfx::ProgramBindings::CreateCopy(*frame.final_pass.cube.sp_program_bindings, {
            { { gfx::Shader::Type::Vertex, "g_mesh_uniforms"  }, { { frame.final_pass.floor.sp_uniforms_buffer  } } },
            { { gfx::Shader::Type::Pixel,  "g_texture"        }, { { m_sp_floor_buffers->GetTexturePtr()        } } },
        });
    }

    // Complete initialization of render context:
    //  - allocate deferred descriptor heaps with calculated sizes
    //  - execute commands to upload resources to GPU
    m_sp_context->CompleteInitialization();
}

void ShadowCubeApp::RenderPass::Release()
{
    sp_state.reset();
}

bool ShadowCubeApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    // Resize screen color and depth textures
    for (ShadowCubeFrame& frame : m_frames)
        frame.final_pass.sp_rt_texture.reset();

    const bool is_resized = GraphicsApp::Resize(frame_size, is_minimized);

    for (ShadowCubeFrame& frame : m_frames)
        frame.final_pass.sp_rt_texture = frame.sp_screen_texture;
    
    if (!is_resized)
        return false;

    // Update viewports and scissor rects state
    m_final_pass.sp_state->SetViewports({ gfx::GetFrameViewport(frame_size) });
    m_final_pass.sp_state->SetScissorRects({ gfx::GetFrameScissorRect(frame_size) });

    m_view_camera.Resize(static_cast<float>(frame_size.width), static_cast<float>(frame_size.height));

    return true;
}

bool ShadowCubeApp::Update()
{
    if (!GraphicsApp::Update())
        return false;

    // Update Model, View, Projection matrices based on scene camera location
    gfx::Matrix44f scale_matrix, scene_view_matrix, scene_proj_matrix;
    cml::matrix_uniform_scale(scale_matrix, m_scene_scale);
    m_view_camera.GetViewProjMatrices(scene_view_matrix, scene_proj_matrix);

    // Update View and Projection matrices based on light camera location
    gfx::Matrix44f light_view_matrix, light_proj_matrix;
    m_light_camera.GetViewProjMatrices(light_view_matrix, light_proj_matrix);
    
    // Prepare shadow transform matrix
    static const gfx::Matrix44f s_shadow_transform_matrix = ([]() -> gfx::Matrix44f
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
    cml::matrix_translation(cube_model_matrix, gfx::Vector3f(0.f, 0.5f, 0.f)); // move up by half of cube model height
    cube_model_matrix = cube_model_matrix * scale_matrix;

    // Update Cube uniforms
    m_sp_cube_buffers->SetFinalPassUniforms(MeshUniforms{
        cube_model_matrix,
        cube_model_matrix * scene_view_matrix * scene_proj_matrix,
        cube_model_matrix * light_view_matrix * light_proj_matrix * s_shadow_transform_matrix
    });
    m_sp_cube_buffers->SetShadowPassUniforms(MeshUniforms{
        cube_model_matrix,
        cube_model_matrix * light_view_matrix * light_proj_matrix,
        gfx::Matrix44f()
    });

    // Update Floor uniforms
    m_sp_floor_buffers->SetFinalPassUniforms(MeshUniforms{
        scale_matrix,
        scale_matrix * scene_view_matrix * scene_proj_matrix,
        scale_matrix * light_view_matrix * light_proj_matrix * s_shadow_transform_matrix
    });
    m_sp_floor_buffers->SetShadowPassUniforms(MeshUniforms{
        scale_matrix,
        scale_matrix * light_view_matrix * light_proj_matrix,
        gfx::Matrix44f()
    });
    
    return true;
}

bool ShadowCubeApp::Render()
{
    // Render only when context is ready
    if (!m_sp_context->ReadyToRender() || !GraphicsApp::Render())
        return false;

    // Wait for previous frame rendering is completed and switch to next frame
    m_sp_context->WaitForGpu(gfx::Context::WaitFor::FramePresented);
    ShadowCubeFrame& frame = GetCurrentFrame();

    // Upload uniform buffers to GPU
    const Data::Size mesh_uniforms_buffer_size = sizeof(MeshUniforms);
    frame.sp_scene_uniforms_buffer->SetData({ {reinterpret_cast<Data::ConstRawPtr>(&m_scene_uniforms), sizeof(SceneUniforms) } });
    frame.shadow_pass.floor.sp_uniforms_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&m_sp_floor_buffers->GetShadowPassUniforms()), mesh_uniforms_buffer_size } });
    frame.shadow_pass.cube.sp_uniforms_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&m_sp_cube_buffers->GetShadowPassUniforms()), mesh_uniforms_buffer_size } });
    frame.final_pass.floor.sp_uniforms_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&m_sp_floor_buffers->GetFinalPassUniforms()), mesh_uniforms_buffer_size } });
    frame.final_pass.cube.sp_uniforms_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&m_sp_cube_buffers->GetFinalPassUniforms()), mesh_uniforms_buffer_size } });

    // Record commands for shadow & final render passes
    RenderScene(m_shadow_pass, frame.shadow_pass, *frame.shadow_pass.sp_rt_texture);
    RenderScene(m_final_pass, frame.final_pass, *frame.shadow_pass.sp_rt_texture);

    // Execute rendering commands and present frame to screen
    m_sp_context->GetRenderCommandQueue().Execute({
        *frame.shadow_pass.sp_cmd_list,
        *frame.final_pass.sp_cmd_list
    });
    m_sp_context->Present();
    
    return true;
}

void ShadowCubeApp::RenderScene(const RenderPass &render_pass, ShadowCubeFrame::PassResources &render_pass_resources, gfx::Texture &shadow_texture)
{
    gfx::RenderCommandList& cmd_list = *render_pass_resources.sp_cmd_list;

    // Reset command list with initial rendering state
    cmd_list.Reset(render_pass.sp_state, render_pass.command_group_name);

    // Draw scene with cube and floor
    m_sp_cube_buffers->Draw(cmd_list, *render_pass_resources.cube.sp_program_bindings);
    m_sp_floor_buffers->Draw(cmd_list, *render_pass_resources.floor.sp_program_bindings);

    if (render_pass.is_final_pass)
    {
        RenderOverlay(cmd_list);
    }

    // Commit command list with present flag in case of final render pass
    cmd_list.Commit();
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
