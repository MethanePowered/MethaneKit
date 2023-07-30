/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: ShadowCubeApp.cpp
Tutorial demonstrating shadow-pass rendering with Methane graphics API

******************************************************************************/

#include "ShadowCubeApp.h"

#include <Methane/Tutorials/AppSettings.h>
#include <Methane/Graphics/CubeMesh.hpp>
#include <Methane/Data/TimeAnimation.h>

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
        GetGraphicsTutorialAppSettings("Methane Shadow Cube", AppOptions::GetDefaultWithColorDepthAndAnim()),
        GetUserInterfaceTutorialAppSettings(AppOptions::GetDefaultWithColorDepthAndAnim()),
        "Methane tutorial of shadow pass rendering")
{
    m_view_camera.ResetOrientation({ { 15.0F, 22.5F, -15.0F }, { 0.0F, 7.5F, 0.0F }, { 0.0F, 1.0F, 0.0F } });

    m_light_camera.ResetOrientation({ { 0.0F,  25.0F, -25.0F }, { 0.0F, 7.5F, 0.0F }, { 0.0F, 1.0F, 0.0F } });
    m_light_camera.SetProjection(gfx::Camera::Projection::Orthogonal);
    m_light_camera.SetParameters({ -300, 300.F, 90.F });
    m_light_camera.Resize(Data::FloatSize(80.F, 80.F));

    // Setup animations
    GetAnimations().emplace_back(std::make_shared<Data::TimeAnimation>(std::bind(&ShadowCubeApp::Animate, this, std::placeholders::_1, std::placeholders::_2)));
}

ShadowCubeApp::~ShadowCubeApp()
{
    // Wait for GPU rendering is completed to release resources
    WaitForRenderComplete();
}

void ShadowCubeApp::Init()
{
    UserInterfaceApp::Init();

    const rhi::RenderContext& render_context = GetRenderContext();
    const rhi::CommandQueue   render_cmd_queue = render_context.GetRenderCommandKit().GetQueue();
    const rhi::RenderContextSettings& context_settings = render_context.GetSettings();
    m_view_camera.Resize(context_settings.frame_size);

    const gfx::Mesh::VertexLayout mesh_layout(Vertex::layout);
    const gfx::CubeMesh<Vertex>   cube_mesh(mesh_layout, 1.F, 1.F, 1.F);
    const gfx::QuadMesh<Vertex>   floor_mesh(mesh_layout, 7.F, 7.F, 0.F, 0, gfx::QuadMesh<Vertex>::FaceType::XZ);

    // Load textures, vertex and index buffers for cube and floor meshes
    constexpr gfx::ImageOptionMask image_options({ gfx::ImageOption::Mipmapped, gfx::ImageOption::SrgbColorSpace });

    m_cube_buffers_ptr = std::make_unique<TexturedMeshBuffers>(render_cmd_queue, cube_mesh, "Cube");
    m_cube_buffers_ptr->SetTexture(GetImageLoader().LoadImageToTexture2D(render_cmd_queue, "MethaneBubbles.jpg", image_options, "Cube Face Texture"));

    m_floor_buffers_ptr = std::make_unique<TexturedMeshBuffers>(render_cmd_queue, floor_mesh, "Floor");
    m_floor_buffers_ptr->SetTexture(GetImageLoader().LoadImageToTexture2D(render_cmd_queue, "MarbleWhite.jpg", image_options, "Floor Texture"));

    const auto constants_data_size      = static_cast<Data::Size>(sizeof(hlslpp::Constants));
    const auto scene_uniforms_data_size = static_cast<Data::Size>(sizeof(hlslpp::SceneUniforms));
    const auto mesh_uniforms_data_size  = static_cast<Data::Size>(sizeof(hlslpp::MeshUniforms));

    // Create constants buffer for frame rendering
    m_const_buffer = render_context.CreateBuffer(rhi::BufferSettings::ForConstantBuffer(constants_data_size));
    m_const_buffer.SetName("Constants Buffer");
    m_const_buffer.SetData(render_cmd_queue, {
        reinterpret_cast<Data::ConstRawPtr>(&m_scene_constants), // NOSONAR
        sizeof(m_scene_constants)
    });

    // Create sampler for cube and floor textures sampling
    m_texture_sampler = render_context.CreateSampler(
        rhi::Sampler::Settings
        {
            rhi::Sampler::Filter  { rhi::Sampler::Filter::MinMag::Linear },
            rhi::Sampler::Address { rhi::Sampler::Address::Mode::ClampToEdge }
        }
    );
    m_texture_sampler.SetName("Texture Sampler");

    // Create sampler for shadow-map texture
    m_shadow_sampler = render_context.CreateSampler(
        rhi::Sampler::Settings
        {
            rhi::Sampler::Filter  { rhi::Sampler::Filter::MinMag::Linear },
            rhi::Sampler::Address { rhi::Sampler::Address::Mode::ClampToEdge }
        }
    );
    m_shadow_sampler.SetName("Shadow Map Sampler");

    // ========= Final Pass Render & View States =========

    const rhi::Shader::EntryFunction    vs_main{ "ShadowCube", "CubeVS" };
    const rhi::Shader::EntryFunction    ps_main{ "ShadowCube", "CubePS" };
    const rhi::Shader::MacroDefinitions textured_shadows_definitions{ { "ENABLE_SHADOWS", "" }, { "ENABLE_TEXTURING", "" } };

    // Create final pass rendering state with program
    rhi::RenderState::Settings final_state_settings
    {
        render_context.CreateProgram(
            rhi::Program::Settings
            {
                rhi::Program::ShaderSet
                {
                    { rhi::ShaderType::Vertex, { Data::ShaderProvider::Get(), vs_main, textured_shadows_definitions } },
                    { rhi::ShaderType::Pixel,  { Data::ShaderProvider::Get(), ps_main, textured_shadows_definitions } },
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
                    { { rhi::ShaderType::Vertex, "g_mesh_uniforms"  }, rhi::ProgramArgumentAccessor::Type::Mutable       },
                    { { rhi::ShaderType::Pixel,  "g_scene_uniforms" }, rhi::ProgramArgumentAccessor::Type::FrameConstant },
                    { { rhi::ShaderType::Pixel,  "g_constants"      }, rhi::ProgramArgumentAccessor::Type::Constant      },
                    { { rhi::ShaderType::Pixel,  "g_shadow_map"     }, rhi::ProgramArgumentAccessor::Type::FrameConstant },
                    { { rhi::ShaderType::Pixel,  "g_shadow_sampler" }, rhi::ProgramArgumentAccessor::Type::Constant      },
                    { { rhi::ShaderType::Pixel,  "g_texture"        }, rhi::ProgramArgumentAccessor::Type::Mutable       },
                    { { rhi::ShaderType::Pixel,  "g_texture_sampler"}, rhi::ProgramArgumentAccessor::Type::Constant      },
                },
                GetScreenRenderPattern().GetAttachmentFormats()
            }
        ),
        GetScreenRenderPattern()
    };
    final_state_settings.program.SetName("Textured, Shadows & Lighting");
    final_state_settings.depth.enabled = true;

    m_final_pass.render_state = render_context.CreateRenderState( final_state_settings);
    m_final_pass.render_state.SetName("Final pass render state");
    m_final_pass.view_state = GetViewState();

    // ========= Shadow Pass Render & View States =========

    // Create shadow-pass render pattern
    m_shadow_pass_pattern = render_context.CreateRenderPattern({
        { }, // No color attachments
        rhi::RenderPattern::DepthAttachment(
            0U, context_settings.depth_stencil_format, 1U,
            rhi::RenderPassAttachment::LoadAction::Clear,
            rhi::RenderPassAttachment::StoreAction::Store,
            context_settings.clear_depth_stencil->first
        ),
        std::nullopt, // No stencil attachment
        rhi::RenderPassAccessMask(rhi::RenderPassAccess::ShaderResources),
        false // intermediate render pass
    });

    // Create shadow-pass rendering state with program
    const rhi::Shader::MacroDefinitions textured_definitions{ { "ENABLE_TEXTURING", "" } };
    rhi::RenderState::Settings shadow_state_settings
    {
        render_context.CreateProgram(
            rhi::Program::Settings
            {
                rhi::Program::ShaderSet
                {
                    { rhi::ShaderType::Vertex, { Data::ShaderProvider::Get(), vs_main, textured_definitions } },
                },
                final_state_settings.program.GetSettings().input_buffer_layouts,
                rhi::ProgramArgumentAccessors
                {
                    { { rhi::ShaderType::Vertex, "g_mesh_uniforms"  }, rhi::ProgramArgumentAccessor::Type::Mutable },
                },
                m_shadow_pass_pattern.GetAttachmentFormats()
            }
        ),
        m_shadow_pass_pattern
    };
    shadow_state_settings.program.SetName("Vertex Only: Textured, Lighting");
    shadow_state_settings.depth.enabled = true;

    m_shadow_pass.render_state = render_context.CreateRenderState( shadow_state_settings);
    m_shadow_pass.render_state.SetName("Shadow-map render state");
    m_shadow_pass.view_state = rhi::ViewState({
        { gfx::GetFrameViewport(g_shadow_map_size)    },
        { gfx::GetFrameScissorRect(g_shadow_map_size) }
    });

    // ========= Per-Frame Data =========

    const rhi::Texture::Settings shadow_texture_settings = rhi::Texture::Settings::ForDepthStencil(
        gfx::Dimensions(g_shadow_map_size),
        context_settings.depth_stencil_format, context_settings.clear_depth_stencil,
        rhi::ResourceUsageMask({ rhi::ResourceUsage::RenderTarget, rhi::ResourceUsage::ShaderRead })
    );

    for(ShadowCubeFrame& frame : GetFrames())
    {
        // Create uniforms buffer with volatile parameters for the whole scene rendering
        frame.scene_uniforms_buffer = render_context.CreateBuffer(rhi::BufferSettings::ForConstantBuffer(scene_uniforms_data_size, false, true));
        frame.scene_uniforms_buffer.SetName(fmt::format("Scene Uniforms Buffer {}", frame.index));

        // ========= Shadow Pass Resources =========

        // Create uniforms buffer for Cube rendering in Shadow pass
        frame.shadow_pass.cube.uniforms_buffer = render_context.CreateBuffer(rhi::BufferSettings::ForConstantBuffer(mesh_uniforms_data_size, false, true));
        frame.shadow_pass.cube.uniforms_buffer.SetName(fmt::format("Cube Uniforms Buffer for Shadow Pass {}", frame.index));

        // Create uniforms buffer for Floor rendering in Shadow pass
        frame.shadow_pass.floor.uniforms_buffer = render_context.CreateBuffer(rhi::BufferSettings::ForConstantBuffer(mesh_uniforms_data_size, false, true));
        frame.shadow_pass.floor.uniforms_buffer.SetName(fmt::format("Floor Uniforms Buffer for Shadow Pass {}", frame.index));

        // Shadow-pass resource bindings for cube rendering
        frame.shadow_pass.cube.program_bindings = shadow_state_settings.program.CreateBindings({
            { { rhi::ShaderType::Vertex, "g_mesh_uniforms"  }, { { frame.shadow_pass.cube.uniforms_buffer.GetInterface() } } },
        }, frame.index);
        frame.shadow_pass.cube.program_bindings.SetName(fmt::format("Cube Shadow-Pass Bindings {}", frame.index));

        // Shadow-pass resource bindings for floor rendering
        frame.shadow_pass.floor.program_bindings = shadow_state_settings.program.CreateBindings({
            { { rhi::ShaderType::Vertex, "g_mesh_uniforms"  }, { { frame.shadow_pass.floor.uniforms_buffer.GetInterface() } } },
        }, frame.index);
        frame.shadow_pass.floor.program_bindings.SetName(fmt::format("Floor Shadow-Pass Bindings {}", frame.index));

        // Create depth texture for shadow map rendering
        frame.shadow_pass.rt_texture = render_context.CreateTexture(shadow_texture_settings);
        frame.shadow_pass.rt_texture.SetName(fmt::format("Shadow Map {}", frame.index));
        
        // Create shadow pass configuration with depth attachment
        frame.shadow_pass.render_pass = m_shadow_pass_pattern.CreateRenderPass({
            { frame.shadow_pass.rt_texture.GetInterface() },
            shadow_texture_settings.dimensions.AsRectSize()
        });
        
        // Create render pass and command list for shadow pass rendering
        frame.shadow_pass.cmd_list = render_cmd_queue.CreateRenderCommandList(frame.shadow_pass.render_pass);
        frame.shadow_pass.cmd_list.SetName(fmt::format("Shadow-Map Rendering {}", frame.index));

        // ========= Final Pass Resources =========

        // Create uniforms buffer for Cube rendering in Final pass
        frame.final_pass.cube.uniforms_buffer = render_context.CreateBuffer(rhi::BufferSettings::ForConstantBuffer(mesh_uniforms_data_size, false, true));
        frame.final_pass.cube.uniforms_buffer.SetName(fmt::format("Cube Uniforms Buffer for Final Pass {}", frame.index));

        // Create uniforms buffer for Floor rendering in Final pass
        frame.final_pass.floor.uniforms_buffer = render_context.CreateBuffer(rhi::BufferSettings::ForConstantBuffer(mesh_uniforms_data_size, false, true));
        frame.final_pass.floor.uniforms_buffer.SetName(fmt::format("Floor Uniforms Buffer for Final Pass {}", frame.index));

        // Final-pass resource bindings for cube rendering
        frame.final_pass.cube.program_bindings = final_state_settings.program.CreateBindings({
            { { rhi::ShaderType::Vertex, "g_mesh_uniforms"  }, { { frame.final_pass.cube.uniforms_buffer.GetInterface()  } } },
            { { rhi::ShaderType::Pixel,  "g_scene_uniforms" }, { { frame.scene_uniforms_buffer.GetInterface()            } } },
            { { rhi::ShaderType::Pixel,  "g_constants"      }, { { m_const_buffer.GetInterface()                         } } },
            { { rhi::ShaderType::Pixel,  "g_shadow_map"     }, { { frame.shadow_pass.rt_texture.GetInterface()           } } },
            { { rhi::ShaderType::Pixel,  "g_shadow_sampler" }, { { m_shadow_sampler.GetInterface()                       } } },
            { { rhi::ShaderType::Pixel,  "g_texture"        }, { { m_cube_buffers_ptr->GetTexture().GetInterface()       } } },
            { { rhi::ShaderType::Pixel,  "g_texture_sampler"}, { { m_texture_sampler.GetInterface()                      } } },
        }, frame.index);
        frame.final_pass.cube.program_bindings.SetName(fmt::format("Cube Final-Pass Bindings {}", frame.index));

        // Final-pass resource bindings for floor rendering - patched a copy of cube bindings
        frame.final_pass.floor.program_bindings = rhi::ProgramBindings(frame.final_pass.cube.program_bindings, {
            { { rhi::ShaderType::Vertex, "g_mesh_uniforms"  }, { { frame.final_pass.floor.uniforms_buffer.GetInterface() } } },
            { { rhi::ShaderType::Pixel,  "g_texture"        }, { { m_floor_buffers_ptr->GetTexture().GetInterface()      } } },
        }, frame.index);
        frame.final_pass.floor.program_bindings.SetName(fmt::format("Floor Final-Pass Bindings {}", frame.index));

        // Bind final pass RT texture and pass to the frame buffer texture and final pass.
        frame.final_pass.rt_texture  = frame.screen_texture;
        frame.final_pass.render_pass = frame.screen_pass;
        
        // Create render pass and command list for final pass rendering
        frame.final_pass.cmd_list = render_cmd_queue.CreateRenderCommandList(frame.final_pass.render_pass);
        frame.final_pass.cmd_list.SetName(fmt::format("Final Scene Rendering {}", frame.index));

        // Rendering command lists sequence
        frame.execute_cmd_list_set = rhi::CommandListSet({
            frame.shadow_pass.cmd_list.GetInterface(),
            frame.final_pass.cmd_list.GetInterface()
        }, frame.index);
    }

    UserInterfaceApp::CompleteInitialization();
}

bool ShadowCubeApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    // Resize screen color and depth textures
    for (ShadowCubeFrame& frame : GetFrames())
        frame.final_pass.rt_texture = {};

    const bool is_resized = UserInterfaceApp::Resize(frame_size, is_minimized);

    for (ShadowCubeFrame& frame : GetFrames())
        frame.final_pass.rt_texture = frame.screen_texture;
    
    if (!is_resized)
        return false;

    m_view_camera.Resize(frame_size);
    return true;
}

bool ShadowCubeApp::Animate(double, double delta_seconds)
{
    m_view_camera.Rotate(m_view_camera.GetOrientation().up, static_cast<float>(delta_seconds * 360.F / 8.F));
    m_light_camera.Rotate(m_light_camera.GetOrientation().up, static_cast<float>(delta_seconds * 360.F / 4.F));
    return true;
}

bool ShadowCubeApp::Update()
{
    if (!UserInterfaceApp::Update())
        return false;

    // Prepare homogenous [-1,1] to texture [0,1] coordinates transformation matrix
    static const hlslpp::float4x4 s_homogen_to_texture_coords_matrix = hlslpp::mul(hlslpp::float4x4::scale(0.5F, -0.5F, 1.F), hlslpp::float4x4::translation(0.5F, 0.5F, 0.F));

    // Update scene uniforms
    m_scene_uniforms.eye_position    = hlslpp::float4(m_view_camera.GetOrientation().eye, 1.F);
    m_scene_uniforms.light_position  = m_light_camera.GetOrientation().eye;

    hlslpp::float4x4 scale_matrix = hlslpp::float4x4::scale(m_scene_scale);

    // Cube model matrix
    hlslpp::float4x4 cube_model_matrix = hlslpp::mul(hlslpp::float4x4::translation(0.F, 0.5F, 0.F), scale_matrix); // move up by half of cube model height

    // Update Cube uniforms
    m_cube_buffers_ptr->SetFinalPassUniforms(hlslpp::MeshUniforms{
        hlslpp::transpose(cube_model_matrix),
        hlslpp::transpose(hlslpp::mul(cube_model_matrix, m_view_camera.GetViewProjMatrix())),
        hlslpp::transpose(hlslpp::mul(hlslpp::mul(cube_model_matrix, m_light_camera.GetViewProjMatrix()), s_homogen_to_texture_coords_matrix))
    });
    m_cube_buffers_ptr->SetShadowPassUniforms(hlslpp::MeshUniforms{
        hlslpp::transpose(cube_model_matrix),
        hlslpp::transpose(hlslpp::mul(cube_model_matrix, m_light_camera.GetViewProjMatrix())),
        hlslpp::float4x4()
    });

    // Update Floor uniforms
    m_floor_buffers_ptr->SetFinalPassUniforms(hlslpp::MeshUniforms{
        hlslpp::transpose(scale_matrix),
        hlslpp::transpose(hlslpp::mul(scale_matrix, m_view_camera.GetViewProjMatrix())),
        hlslpp::transpose(hlslpp::mul(hlslpp::mul(scale_matrix, m_light_camera.GetViewProjMatrix()), s_homogen_to_texture_coords_matrix))
    });
    m_floor_buffers_ptr->SetShadowPassUniforms(hlslpp::MeshUniforms{
        hlslpp::transpose(scale_matrix),
        hlslpp::transpose(hlslpp::mul(scale_matrix, m_light_camera.GetViewProjMatrix())),
        hlslpp::float4x4()
    });
    
    return true;
}

bool ShadowCubeApp::Render()
{
    if (!UserInterfaceApp::Render())
        return false;

    // Upload uniform buffers to GPU
    const ShadowCubeFrame& frame = GetCurrentFrame();
    const rhi::CommandQueue render_cmd_queue = GetRenderContext().GetRenderCommandKit().GetQueue();
    frame.scene_uniforms_buffer.SetData(render_cmd_queue, m_scene_uniforms_subresource);
    frame.shadow_pass.floor.uniforms_buffer.SetData(render_cmd_queue, m_floor_buffers_ptr->GetShadowPassUniformsSubresource());
    frame.shadow_pass.cube.uniforms_buffer.SetData(render_cmd_queue, m_cube_buffers_ptr->GetShadowPassUniformsSubresource());
    frame.final_pass.floor.uniforms_buffer.SetData(render_cmd_queue, m_floor_buffers_ptr->GetFinalPassUniformsSubresource());
    frame.final_pass.cube.uniforms_buffer.SetData(render_cmd_queue, m_cube_buffers_ptr->GetFinalPassUniformsSubresource());

    // Record commands for shadow & final render passes
    RenderScene(m_shadow_pass, frame.shadow_pass);
    RenderScene(m_final_pass, frame.final_pass);

    // Execute rendering commands and present frame to screen
    GetRenderContext().GetRenderCommandKit().GetQueue().Execute(frame.execute_cmd_list_set);
    GetRenderContext().Present();
    
    return true;
}

void ShadowCubeApp::RenderScene(const RenderPassState& render_pass, const ShadowCubeFrame::PassResources& render_pass_resources) const
{
    const rhi::RenderCommandList& cmd_list = render_pass_resources.cmd_list;

    // Reset command list with initial rendering state
    cmd_list.ResetWithState(render_pass.render_state, &render_pass.debug_group);
    cmd_list.SetViewState(render_pass.view_state);

    // Draw scene with cube and floor
    m_cube_buffers_ptr->Draw(cmd_list, render_pass_resources.cube.program_bindings);
    m_floor_buffers_ptr->Draw(cmd_list, render_pass_resources.floor.program_bindings);

    if (render_pass.is_final_pass)
    {
        RenderOverlay(cmd_list);
    }

    cmd_list.Commit();
}

void ShadowCubeApp::OnContextReleased(rhi::IContext& context)
{
    m_final_pass.Release();
    m_shadow_pass.Release();

    m_floor_buffers_ptr.reset();
    m_cube_buffers_ptr.reset();

    m_shadow_sampler = {};
    m_texture_sampler = {};
    m_const_buffer = {};
    m_shadow_pass_pattern = {};

    UserInterfaceApp::OnContextReleased(context);
}

ShadowCubeApp::RenderPassState::RenderPassState(bool is_final_pass, const std::string& debug_group_name)
    : is_final_pass(is_final_pass)
    , debug_group(META_DEBUG_GROUP_CREATE(debug_group_name)) // NOSONAR
{
    META_UNUSED(debug_group_name);
}

void ShadowCubeApp::RenderPassState::Release()
{
    render_state = {};
    view_state = {};
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::ShadowCubeApp().Run({ argc, argv });
}
