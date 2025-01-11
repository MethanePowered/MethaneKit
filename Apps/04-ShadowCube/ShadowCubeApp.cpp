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
#include <Methane/Data/TimeAnimation.hpp>

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

static const gfx::FrameSize    g_shadow_map_size(1024, 1024);
static const hlslpp::Constants g_scene_constants{
    { 1.F, 1.F, 0.74F, 1.F }, // - light_color
    700.F,                    // - light_power
    0.04F,                    // - light_ambient_factor
    30.F                      // - light_specular_factor
};

static_assert(sizeof(hlslpp::Constants) % 16 == 0,     "Size of Constants struct should have 16 byte alignment!");
static_assert(sizeof(hlslpp::SceneUniforms) % 16 == 0, "Size of SceneUniforms struct should have 16 byte alignment!");
static_assert(sizeof(hlslpp::MeshUniforms) % 16 == 0,  "Size of MeshUniforms struct should have 16 byte alignment!");

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
    GetAnimations().emplace_back(Data::MakeTimeAnimationPtr([this](double elapsed_seconds, double delta_seconds)
    {
        return Animate(elapsed_seconds, delta_seconds);
    }));
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
                    META_PROGRAM_ARG_ROOT_BUFFER_CONSTANT(rhi::ShaderType::Pixel, "g_constants"),
                    META_PROGRAM_ARG_ROOT_BUFFER_FRAME_CONSTANT(rhi::ShaderType::Pixel, "g_scene_uniforms"),
                    META_PROGRAM_ARG_ROOT_BUFFER_MUTABLE(rhi::ShaderType::Vertex, "g_mesh_uniforms")
                },
                GetScreenRenderPattern().GetAttachmentFormats()
            }
        ),
        GetScreenRenderPattern()
    };
    final_state_settings.program.SetName("Texturing, Shadows & Lighting");
    final_state_settings.depth.enabled = true;

    m_final_pass.render_state = render_context.CreateRenderState( final_state_settings);
    m_final_pass.render_state.SetName("Final Pass Render State");
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
                    META_PROGRAM_ARG_ROOT_BUFFER_MUTABLE(rhi::ShaderType::Vertex, "g_mesh_uniforms")
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
        // ========= Shadow Pass Resources =========

        // Shadow-pass resource bindings for cube rendering
        ShadowCubeFrame::PassResources::ProgramBindings& shadow_cube_binds = frame.shadow_pass.cube_bindings;
        shadow_cube_binds.program_bindings = shadow_state_settings.program.CreateBindings({ }, frame.index);
        shadow_cube_binds.program_bindings.SetName(fmt::format("Cube Shadow-Pass Bindings {}", frame.index));
        shadow_cube_binds.mesh_uniforms_binding_ptr = &shadow_cube_binds.program_bindings.Get({ rhi::ShaderType::Vertex, "g_mesh_uniforms" });

        // Shadow-pass resource bindings for floor rendering
        ShadowCubeFrame::PassResources::ProgramBindings& shadow_floor_binds = frame.shadow_pass.floor_bindings;
        shadow_floor_binds.program_bindings = shadow_state_settings.program.CreateBindings({ }, frame.index);
        shadow_floor_binds.program_bindings.SetName(fmt::format("Floor Shadow-Pass Bindings {}", frame.index));
        shadow_floor_binds.mesh_uniforms_binding_ptr = &shadow_floor_binds.program_bindings.Get({ rhi::ShaderType::Vertex, "g_mesh_uniforms" });

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

        // Final-pass resource bindings for cube rendering
        ShadowCubeFrame::PassResources::ProgramBindings& final_cube_binds = frame.final_pass.cube_bindings;
        final_cube_binds.program_bindings = final_state_settings.program.CreateBindings({
            { { rhi::ShaderType::Pixel,  "g_constants"      }, rhi::RootConstant(g_scene_constants)               },
            { { rhi::ShaderType::Pixel,  "g_shadow_map"     }, frame.shadow_pass.rt_texture.GetResourceView()     },
            { { rhi::ShaderType::Pixel,  "g_shadow_sampler" }, m_shadow_sampler.GetResourceView()                 },
            { { rhi::ShaderType::Pixel,  "g_texture"        }, m_cube_buffers_ptr->GetTexture().GetResourceView() },
            { { rhi::ShaderType::Pixel,  "g_texture_sampler"}, m_texture_sampler.GetResourceView()                },
        }, frame.index);
        final_cube_binds.program_bindings.SetName(fmt::format("Cube Final-Pass Bindings {}", frame.index));
        final_cube_binds.scene_uniforms_binding_ptr = &final_cube_binds.program_bindings.Get({ rhi::ShaderType::Pixel, "g_scene_uniforms" });
        final_cube_binds.mesh_uniforms_binding_ptr  = &final_cube_binds.program_bindings.Get({ rhi::ShaderType::Vertex, "g_mesh_uniforms" });

        // Final-pass resource bindings for floor rendering - patched a copy of cube bindings
        ShadowCubeFrame::PassResources::ProgramBindings& final_floor_binds = frame.final_pass.floor_bindings;
        final_floor_binds.program_bindings = rhi::ProgramBindings(frame.final_pass.cube_bindings.program_bindings, {
            { { rhi::ShaderType::Pixel,  "g_texture" }, m_floor_buffers_ptr->GetTexture().GetResourceView() },
        }, frame.index);
        final_floor_binds.program_bindings.SetName(fmt::format("Floor Final-Pass Bindings {}", frame.index));
        final_floor_binds.scene_uniforms_binding_ptr = &final_floor_binds.program_bindings.Get({ rhi::ShaderType::Pixel,  "g_scene_uniforms" });
        final_floor_binds.mesh_uniforms_binding_ptr  = &final_floor_binds.program_bindings.Get({ rhi::ShaderType::Vertex, "g_mesh_uniforms"  });

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

    const hlslpp::SceneUniforms scene_uniforms{
        hlslpp::float4(m_view_camera.GetOrientation().eye, 1.F), // eye_position
        hlslpp::float4(m_light_camera.GetOrientation().eye, 1.F) // light_position
    };
    const rhi::RootConstant scene_uniforms_constant(scene_uniforms);

    // Prepare homogenous [-1,1] to texture [0,1] coordinates transformation matrix
    static const hlslpp::float4x4 s_homogen_to_texture_coords_matrix = hlslpp::mul(
        hlslpp::float4x4::scale(0.5F, -0.5F, 1.F),
        hlslpp::float4x4::translation(0.5F, 0.5F, 0.F)
    );

    const hlslpp::float4x4 scale_matrix = hlslpp::float4x4::scale(15.F);
    const hlslpp::float4x4 cube_model_matrix = hlslpp::mul(hlslpp::float4x4::translation(0.F, 0.5F, 0.F), scale_matrix);

    const ShadowCubeFrame& frame = GetCurrentFrame();

    // Update Cube uniforms
    const hlslpp::MeshUniforms final_cube_uniforms{
        hlslpp::transpose(cube_model_matrix),
        hlslpp::transpose(hlslpp::mul(cube_model_matrix, m_view_camera.GetViewProjMatrix())),
        hlslpp::transpose(hlslpp::mul(hlslpp::mul(cube_model_matrix, m_light_camera.GetViewProjMatrix()), s_homogen_to_texture_coords_matrix))
    };
    const hlslpp::MeshUniforms shadow_cube_uniforms{
        hlslpp::transpose(cube_model_matrix),
        hlslpp::transpose(hlslpp::mul(cube_model_matrix, m_light_camera.GetViewProjMatrix())),
        hlslpp::float4x4()
    };
    frame.final_pass.cube_bindings.scene_uniforms_binding_ptr->SetRootConstant(scene_uniforms_constant);
    frame.final_pass.cube_bindings.mesh_uniforms_binding_ptr->SetRootConstant(rhi::RootConstant(final_cube_uniforms));
    frame.shadow_pass.cube_bindings.mesh_uniforms_binding_ptr->SetRootConstant(rhi::RootConstant(shadow_cube_uniforms));

    // Update Floor uniforms
    const hlslpp::MeshUniforms final_floor_uniforms{
        hlslpp::transpose(scale_matrix),
        hlslpp::transpose(hlslpp::mul(scale_matrix, m_view_camera.GetViewProjMatrix())),
        hlslpp::transpose(hlslpp::mul(hlslpp::mul(scale_matrix, m_light_camera.GetViewProjMatrix()), s_homogen_to_texture_coords_matrix))
    };
    const hlslpp::MeshUniforms shadow_floor_uniforms{
        hlslpp::transpose(scale_matrix),
        hlslpp::transpose(hlslpp::mul(scale_matrix, m_light_camera.GetViewProjMatrix())),
        hlslpp::float4x4()
    };
    frame.final_pass.floor_bindings.scene_uniforms_binding_ptr->SetRootConstant(scene_uniforms_constant);
    frame.final_pass.floor_bindings.mesh_uniforms_binding_ptr->SetRootConstant(rhi::RootConstant(final_floor_uniforms));
    frame.shadow_pass.floor_bindings.mesh_uniforms_binding_ptr->SetRootConstant(rhi::RootConstant(shadow_floor_uniforms));

    return true;
}

bool ShadowCubeApp::Render()
{
    if (!UserInterfaceApp::Render())
        return false;

    // Record commands for shadow & final render passes
    const ShadowCubeFrame& frame = GetCurrentFrame();
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
    m_cube_buffers_ptr->Draw(cmd_list, render_pass_resources.cube_bindings.program_bindings);
    m_floor_buffers_ptr->Draw(cmd_list, render_pass_resources.floor_bindings.program_bindings);

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
