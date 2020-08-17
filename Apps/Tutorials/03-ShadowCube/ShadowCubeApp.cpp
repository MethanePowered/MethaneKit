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

#include <Methane/Samples/AppSettings.hpp>
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

    inline static const gfx::Mesh::VertexLayout layout{
        gfx::Mesh::VertexField::Position,
        gfx::Mesh::VertexField::Normal,
        gfx::Mesh::VertexField::TexCoord,
    };
};

static const gfx::FrameSize g_shadow_map_size(1024, 1024);

ShadowCubeApp::ShadowCubeApp()
    : UserInterfaceApp(
        Samples::GetGraphicsAppSettings("Methane Shadow Cube"), {},
        "Methane tutorial of shadow pass rendering")
    , m_scene_scale(15.f)
    , m_scene_constants(                                // Shader constants:
        {                                               // ================
            gfx::Color4f(1.f, 1.f, 0.74f, 1.f),         // - light_color
            700.f,                                      // - light_power
            0.04f,                                      // - light_ambient_factor
            30.f                                        // - light_specular_factor
        })
    , m_shadow_pass(false, "Shadow Render Pass")
    , m_final_pass(true, "Final Render Pass")
{
    m_view_camera.ResetOrientation({ { 15.0f, 22.5f, -15.0f }, { 0.0f, 7.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } });

    m_light_camera.ResetOrientation({ { 0.0f,  25.0f, -25.0f }, { 0.0f, 7.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } });
    m_light_camera.SetProjection(gfx::Camera::Projection::Orthogonal);
    m_light_camera.SetParameters({ -300, 300.f, 90.f });
    m_light_camera.Resize({ 80.f, 80.f });

    // Setup animations
    GetAnimations().emplace_back(std::make_shared<Data::TimeAnimation>(std::bind(&ShadowCubeApp::Animate, this, std::placeholders::_1, std::placeholders::_2)));
}

ShadowCubeApp::~ShadowCubeApp()
{
    // Wait for GPU rendering is completed to release resources
    GetRenderContext().WaitForGpu(gfx::Context::WaitFor::RenderComplete);
}

void ShadowCubeApp::Init()
{
    UserInterfaceApp::Init();

    const gfx::RenderContext::Settings& context_settings = GetRenderContext().GetSettings();
    m_view_camera.Resize({
        static_cast<float>(context_settings.frame_size.width),
        static_cast<float>(context_settings.frame_size.height)
    });

    const gfx::Mesh::VertexLayout mesh_layout(Vertex::layout);
    const gfx::CubeMesh<Vertex>   cube_mesh(mesh_layout, 1.f, 1.f, 1.f);
    const gfx::QuadMesh<Vertex>   floor_mesh(mesh_layout, 7.f, 7.f, 0.f, 0, gfx::QuadMesh<Vertex>::FaceType::XZ);

    // Load textures, vertex and index buffers for cube and floor meshes
    const gfx::ImageLoader::Options::Mask image_options = gfx::ImageLoader::Options::Mipmapped
                                                        | gfx::ImageLoader::Options::SrgbColorSpace;

    m_sp_cube_buffers  = std::make_unique<TexturedMeshBuffers>(GetRenderContext(), cube_mesh, "Cube");
    m_sp_cube_buffers->SetTexture(GetImageLoader().LoadImageToTexture2D(GetRenderContext(), "Textures/MethaneBubbles.jpg", image_options));

    m_sp_floor_buffers = std::make_unique<TexturedMeshBuffers>(GetRenderContext(), floor_mesh, "Floor");
    m_sp_floor_buffers->SetTexture(GetImageLoader().LoadImageToTexture2D(GetRenderContext(), "Textures/MarbleWhite.jpg", image_options));

    const Data::Size constants_data_size      = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(Constants)));
    const Data::Size scene_uniforms_data_size = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(SceneUniforms)));
    const Data::Size mesh_uniforms_data_size  = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(MeshUniforms)));

    // Create constants buffer for frame rendering
    m_sp_const_buffer = gfx::Buffer::CreateConstantBuffer(GetRenderContext(), constants_data_size);
    m_sp_const_buffer->SetName("Constants Buffer");
    m_sp_const_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&m_scene_constants), sizeof(m_scene_constants) } });

    // Create sampler for cube and floor textures sampling
    m_sp_texture_sampler = gfx::Sampler::Create(GetRenderContext(),
        gfx::Sampler::Settings
        {
            gfx::Sampler::Filter  { gfx::Sampler::Filter::MinMag::Linear },
            gfx::Sampler::Address { gfx::Sampler::Address::Mode::ClampToEdge }
        }
    );
    m_sp_texture_sampler->SetName("Texture Sampler");

    // Create sampler for shadow-map texture
    m_sp_shadow_sampler = gfx::Sampler::Create(GetRenderContext(),
        gfx::Sampler::Settings
        {
            gfx::Sampler::Filter  { gfx::Sampler::Filter::MinMag::Linear },
            gfx::Sampler::Address { gfx::Sampler::Address::Mode::ClampToEdge }
        }
    );
    m_sp_shadow_sampler->SetName("Shadow Map Sampler");

    // ========= Final Pass objects =========

    const gfx::Shader::EntryFunction    vs_main{ "ShadowCube", "CubeVS" };
    const gfx::Shader::EntryFunction    ps_main{ "ShadowCube", "CubePS" };
    const gfx::Shader::MacroDefinitions textured_shadows_definitions{ { "ENABLE_SHADOWS", "" }, { "ENABLE_TEXTURING", "" } };

    // Create final pass rendering state with program
    gfx::RenderState::Settings final_state_settings;
    final_state_settings.sp_program = gfx::Program::Create(GetRenderContext(),
        gfx::Program::Settings
        {
            gfx::Program::Shaders
            {
                gfx::Shader::CreateVertex(GetRenderContext(), { Data::ShaderProvider::Get(), vs_main, textured_shadows_definitions }),
                gfx::Shader::CreatePixel(GetRenderContext(),  { Data::ShaderProvider::Get(), ps_main, textured_shadows_definitions }),
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
    final_state_settings.depth.enabled  = true;
    m_final_pass.sp_render_state        = gfx::RenderState::Create(GetRenderContext(), final_state_settings);
    m_final_pass.sp_render_state->SetName("Final pass render state");

    m_final_pass.sp_view_state = GetViewStatePtr();

    // ========= Shadow Pass objects =========
    
    gfx::Texture::Settings        shadow_texture_settings = gfx::Texture::Settings::DepthStencilBuffer(g_shadow_map_size, context_settings.depth_stencil_format, gfx::Texture::Usage::RenderTarget | gfx::Texture::Usage::ShaderRead);
    gfx::Shader::MacroDefinitions textured_definitions    { { "ENABLE_TEXTURING", "" } };

    // Create shadow-pass rendering state with program
    gfx::RenderState::Settings shadow_state_settings;
    shadow_state_settings.sp_program = gfx::Program::Create(GetRenderContext(),
        gfx::Program::Settings
        {
            gfx::Program::Shaders
            {
                gfx::Shader::CreateVertex(GetRenderContext(), { Data::ShaderProvider::Get(), vs_main, textured_definitions }),
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
    shadow_state_settings.depth.enabled = true;
    m_shadow_pass.sp_render_state       = gfx::RenderState::Create(GetRenderContext(), shadow_state_settings);
    m_shadow_pass.sp_render_state->SetName("Shadow-map render state");

    m_shadow_pass.sp_view_state = gfx::ViewState::Create({
        { gfx::GetFrameViewport(g_shadow_map_size)    },
        { gfx::GetFrameScissorRect(g_shadow_map_size) }
    });

    // ========= Per-Frame Data =========
    for(ShadowCubeFrame& frame : GetFrames())
    {
        // Create uniforms buffer with volatile parameters for the whole scene rendering
        frame.sp_scene_uniforms_buffer = gfx::Buffer::CreateVolatileBuffer(GetRenderContext(), scene_uniforms_data_size);
        frame.sp_scene_uniforms_buffer->SetName(IndexedName("Scene Uniforms Buffer", frame.index));

        // ========= Shadow Pass data =========

        // Create uniforms buffer for Cube rendering in Shadow pass
        frame.shadow_pass.cube.sp_uniforms_buffer = gfx::Buffer::CreateVolatileBuffer(GetRenderContext(), mesh_uniforms_data_size);
        frame.shadow_pass.cube.sp_uniforms_buffer->SetName(IndexedName("Cube Uniforms Buffer for Shadow Pass", frame.index));

        // Create uniforms buffer for Floor rendering in Shadow pass
        frame.shadow_pass.floor.sp_uniforms_buffer = gfx::Buffer::CreateVolatileBuffer(GetRenderContext(), mesh_uniforms_data_size);
        frame.shadow_pass.floor.sp_uniforms_buffer->SetName(IndexedName("Floor Uniforms Buffer for Shadow Pass", frame.index));

        // Create depth texture for shadow map rendering
        frame.shadow_pass.sp_rt_texture = gfx::Texture::CreateRenderTarget(GetRenderContext(), shadow_texture_settings);
        frame.shadow_pass.sp_rt_texture->SetName(IndexedName("Shadow Map", frame.index));
        
        // Create shadow pass configuration with depth attachment
        frame.shadow_pass.sp_pass = gfx::RenderPass::Create(GetRenderContext(), {
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
            gfx::RenderPass::Access::ShaderResources,
            false // intermediate render pass
        });
        
        // Create render pass and command list for shadow pass rendering
        frame.shadow_pass.sp_cmd_list = gfx::RenderCommandList::Create(GetRenderContext().GetRenderCommandQueue(), *frame.shadow_pass.sp_pass);
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
        frame.final_pass.cube.sp_uniforms_buffer = gfx::Buffer::CreateVolatileBuffer(GetRenderContext(), mesh_uniforms_data_size);
        frame.final_pass.cube.sp_uniforms_buffer->SetName(IndexedName("Cube Uniforms Buffer for Final Pass", frame.index));

        // Create uniforms buffer for Floor rendering in Final pass
        frame.final_pass.floor.sp_uniforms_buffer = gfx::Buffer::CreateVolatileBuffer(GetRenderContext(), mesh_uniforms_data_size);
        frame.final_pass.floor.sp_uniforms_buffer->SetName(IndexedName("Floor Uniforms Buffer for Final Pass", frame.index));

        // Bind final pass RT texture and pass to the frame buffer texture and final pass.
        frame.final_pass.sp_rt_texture = frame.sp_screen_texture;
        frame.final_pass.sp_pass       = frame.sp_screen_pass;
        
        // Create render pass and command list for final pass rendering
        frame.final_pass.sp_cmd_list = gfx::RenderCommandList::Create(GetRenderContext().GetRenderCommandQueue(), *frame.final_pass.sp_pass);
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

        // Rendering command lists sequence
        frame.sp_execute_cmd_list_set = gfx::CommandListSet::Create({
            *frame.shadow_pass.sp_cmd_list,
            *frame.final_pass.sp_cmd_list
        });
    }

    UserInterfaceApp::CompleteInitialization();
}

void ShadowCubeApp::RenderPass::Release()
{
    sp_render_state.reset();
    sp_view_state.reset();
}

bool ShadowCubeApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    // Resize screen color and depth textures
    for (ShadowCubeFrame& frame : GetFrames())
        frame.final_pass.sp_rt_texture.reset();

    const bool is_resized = UserInterfaceApp::Resize(frame_size, is_minimized);

    for (ShadowCubeFrame& frame : GetFrames())
        frame.final_pass.sp_rt_texture = frame.sp_screen_texture;
    
    if (!is_resized)
        return false;

    m_view_camera.Resize({
        static_cast<float>(frame_size.width),
        static_cast<float>(frame_size.height)
    });

    return true;
}

bool ShadowCubeApp::Update()
{
    if (!UserInterfaceApp::Update())
        return false;

    gfx::Matrix44f scale_matrix;
    cml::matrix_uniform_scale(scale_matrix, m_scene_scale);
    
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
        cube_model_matrix * m_view_camera.GetViewProjMatrix(),
        cube_model_matrix * m_light_camera.GetViewProjMatrix() * s_shadow_transform_matrix
    });
    m_sp_cube_buffers->SetShadowPassUniforms(MeshUniforms{
        cube_model_matrix,
        cube_model_matrix * m_light_camera.GetViewProjMatrix(),
        gfx::Matrix44f()
    });

    // Update Floor uniforms
    m_sp_floor_buffers->SetFinalPassUniforms(MeshUniforms{
        scale_matrix,
        scale_matrix * m_view_camera.GetViewProjMatrix(),
        scale_matrix * m_light_camera.GetViewProjMatrix() * s_shadow_transform_matrix
    });
    m_sp_floor_buffers->SetShadowPassUniforms(MeshUniforms{
        scale_matrix,
        scale_matrix * m_light_camera.GetViewProjMatrix(),
        gfx::Matrix44f()
    });
    
    return true;
}

bool ShadowCubeApp::Animate(double, double delta_seconds)
{
    m_view_camera.Rotate(m_view_camera.GetOrientation().up, static_cast<float>(delta_seconds * 360.f / 8.f));
    m_light_camera.Rotate(m_light_camera.GetOrientation().up, static_cast<float>(delta_seconds * 360.f / 4.f));
    return true;
}

bool ShadowCubeApp::Render()
{
    if (!UserInterfaceApp::Render())
        return false;

    // Upload uniform buffers to GPU
    ShadowCubeFrame& frame = GetCurrentFrame();
    frame.sp_scene_uniforms_buffer->SetData(m_scene_uniforms_subresources);
    frame.shadow_pass.floor.sp_uniforms_buffer->SetData(m_sp_floor_buffers->GetShadowPassUniformsSubresources());
    frame.shadow_pass.cube.sp_uniforms_buffer->SetData(m_sp_cube_buffers->GetShadowPassUniformsSubresources());
    frame.final_pass.floor.sp_uniforms_buffer->SetData(m_sp_floor_buffers->GetFinalPassUniformsSubresources());
    frame.final_pass.cube.sp_uniforms_buffer->SetData(m_sp_cube_buffers->GetFinalPassUniformsSubresources());

    // Record commands for shadow & final render passes
    RenderScene(m_shadow_pass, frame.shadow_pass);
    RenderScene(m_final_pass, frame.final_pass);

    // Execute rendering commands and present frame to screen
    GetRenderContext().GetRenderCommandQueue().Execute(*frame.sp_execute_cmd_list_set);
    GetRenderContext().Present();
    
    return true;
}

void ShadowCubeApp::RenderScene(const RenderPass &render_pass, ShadowCubeFrame::PassResources &render_pass_resources)
{
    gfx::RenderCommandList& cmd_list = *render_pass_resources.sp_cmd_list;

    // Reset command list with initial rendering state
    cmd_list.Reset(render_pass.sp_render_state, render_pass.sp_debug_group.get());
    cmd_list.SetViewState(*render_pass.sp_view_state);

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

void ShadowCubeApp::OnContextReleased(gfx::Context& context)
{
    m_final_pass.Release();
    m_shadow_pass.Release();

    m_sp_floor_buffers.reset();
    m_sp_cube_buffers.reset();
    m_sp_shadow_sampler.reset();
    m_sp_texture_sampler.reset();
    m_sp_const_buffer.reset();

    UserInterfaceApp::OnContextReleased(context);
}

ShadowCubeApp::RenderPass::RenderPass(bool is_final_pass, std::string debug_group_name)
    : is_final_pass(is_final_pass)
    , sp_debug_group(META_DEBUG_GROUP_CREATE(std::move(debug_group_name)))
{
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::ShadowCubeApp().Run({ argc, argv });
}
