/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: TexturedCubeApp.cpp
Tutorial demonstrating textured cube rendering with Methane graphics API

******************************************************************************/

#include "TexturedCubeApp.h"

#include <Methane/Tutorials/AppSettings.h>
#include <Methane/Graphics/CubeMesh.hpp>
#include <Methane/Graphics/TypeConverters.hpp>
#include <Methane/Data/TimeAnimation.h>

namespace Methane::Tutorials
{

struct CubeVertex
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

TexturedCubeApp::TexturedCubeApp()
    : UserInterfaceApp(
        GetGraphicsTutorialAppSettings("Methane Textured Cube", AppOptions::GetDefaultWithColorOnlyAndAnim()),
        GetUserInterfaceTutorialAppSettings(AppOptions::GetDefaultWithColorOnlyAndAnim()),
        "Methane tutorial of textured cube rendering")
{
    m_shader_uniforms.light_position = hlslpp::float3(0.F, 20.F, -25.F);
    m_camera.ResetOrientation({ { 13.0F, 13.0F, -13.0F }, { 0.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } });

    m_shader_uniforms.model_matrix = hlslpp::float4x4::scale(m_cube_scale);

    // Setup animations
    GetAnimations().emplace_back(std::make_shared<Data::TimeAnimation>(std::bind(&TexturedCubeApp::Animate, this, std::placeholders::_1, std::placeholders::_2)));
}

TexturedCubeApp::~TexturedCubeApp()
{
    // Wait for GPU rendering is completed to release resources
    WaitForRenderComplete();
}

void TexturedCubeApp::Init()
{
    UserInterfaceApp::Init();

    const rhi::CommandQueue render_cmd_queue = GetRenderContext().GetRenderCommandKit().GetQueue();
    m_camera.Resize(GetRenderContext().GetSettings().frame_size);

    // Create vertex buffer for cube mesh
    const gfx::CubeMesh<CubeVertex> cube_mesh(CubeVertex::layout);
    const Data::Size vertex_data_size   = cube_mesh.GetVertexDataSize();
    const Data::Size  vertex_size       = cube_mesh.GetVertexSize();
    rhi::Buffer vertex_buffer = GetRenderContext().CreateBuffer(rhi::BufferSettings::ForVertexBuffer(vertex_data_size, vertex_size));
    vertex_buffer.SetName("Cube Vertex Buffer");
    vertex_buffer.SetData(render_cmd_queue, {
        reinterpret_cast<Data::ConstRawPtr>(cube_mesh.GetVertices().data()), // NOSONAR
        vertex_data_size
    });
    m_vertex_buffer_set = rhi::BufferSet(rhi::BufferType::Vertex, { vertex_buffer });

    // Create index buffer for cube mesh
    const Data::Size index_data_size = cube_mesh.GetIndexDataSize();
    const gfx::PixelFormat index_format = gfx::GetIndexFormat(cube_mesh.GetIndex(0));
    m_index_buffer = GetRenderContext().CreateBuffer(rhi::BufferSettings::ForIndexBuffer(index_data_size, index_format));
    m_index_buffer.SetName("Cube Index Buffer");
    m_index_buffer.SetData(render_cmd_queue, {
        reinterpret_cast<Data::ConstRawPtr>(cube_mesh.GetIndices().data()), // NOSONAR
        index_data_size
    });

    // Create constants buffer for frame rendering
    const auto constants_data_size = static_cast<Data::Size>(sizeof(m_shader_constants));
    m_const_buffer = GetRenderContext().CreateBuffer(rhi::BufferSettings::ForConstantBuffer(constants_data_size));
    m_const_buffer.SetName("Constants Buffer");
    m_const_buffer.SetData(render_cmd_queue, {
        reinterpret_cast<Data::ConstRawPtr>(&m_shader_constants), // NOSONAR
        constants_data_size
    });

    // Create render state with program
    m_render_state = GetRenderContext().CreateRenderState(
        rhi::RenderState::Settings
        {
            GetRenderContext().CreateProgram(
                rhi::Program::Settings
                {
                    rhi::Program::ShaderSet
                    {
                        { rhi::ShaderType::Vertex, { Data::ShaderProvider::Get(), { "TexturedCube", "CubeVS" } } },
                        { rhi::ShaderType::Pixel,  { Data::ShaderProvider::Get(), { "TexturedCube", "CubePS" } } },
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
                        { { rhi::ShaderType::All,   "g_uniforms"  }, rhi::ProgramArgumentAccessor::Type::FrameConstant },
                        { { rhi::ShaderType::Pixel, "g_constants" }, rhi::ProgramArgumentAccessor::Type::Constant },
                        { { rhi::ShaderType::Pixel, "g_texture"   }, rhi::ProgramArgumentAccessor::Type::Constant },
                        { { rhi::ShaderType::Pixel, "g_sampler"   }, rhi::ProgramArgumentAccessor::Type::Constant },
                    },
                    GetScreenRenderPattern().GetAttachmentFormats()
                }
            ),
            GetScreenRenderPattern()
        }
    );
    m_render_state.GetSettings().program_ptr->SetName("Textured Phong Lighting");
    m_render_state.SetName("Final FB Render Pipeline State");

    // Load texture image from file
    constexpr gfx::ImageOptionMask image_options({ gfx::ImageOption::Mipmapped, gfx::ImageOption::SrgbColorSpace });
    m_cube_texture = GetImageLoader().LoadImageToTexture2D(render_cmd_queue, "MethaneBubbles.jpg", image_options, "Cube Face Texture");

    // Create sampler for image texture
    m_texture_sampler = GetRenderContext().CreateSampler(
        rhi::Sampler::Settings
        {
            rhi::Sampler::Filter  { rhi::Sampler::Filter::MinMag::Linear },
            rhi::Sampler::Address { rhi::Sampler::Address::Mode::ClampToEdge }
        }
    );

    // Create frame buffer resources
    const auto uniforms_data_size = static_cast<Data::Size>(sizeof(m_shader_uniforms));
    for(TexturedCubeFrame& frame : GetFrames())
    {
        // Create uniforms buffer with volatile parameters for frame rendering
        frame.uniforms_buffer = GetRenderContext().CreateBuffer(rhi::BufferSettings::ForConstantBuffer(uniforms_data_size, false, true));
        frame.uniforms_buffer.SetName(fmt::format("Uniforms Buffer {}", frame.index));

        // Configure program resource bindings
        frame.program_bindings = m_render_state.GetProgram().CreateBindings({
            { { rhi::ShaderType::All,   "g_uniforms"  }, { { frame.uniforms_buffer.GetInterface() } } },
            { { rhi::ShaderType::Pixel, "g_constants" }, { { m_const_buffer.GetInterface()        } } },
            { { rhi::ShaderType::Pixel, "g_texture"   }, { { m_cube_texture.GetInterface()        } } },
            { { rhi::ShaderType::Pixel, "g_sampler"   }, { { m_texture_sampler.GetInterface()     } } },
        }, frame.index);
        frame.program_bindings.SetName(fmt::format("Cube Bindings {}", frame.index));

        // Create command list for rendering
        frame.render_cmd_list = render_cmd_queue.CreateRenderCommandList(frame.screen_pass);
        frame.render_cmd_list.SetName(fmt::format("Cube Rendering {}", frame.index));
        frame.execute_cmd_list_set = rhi::CommandListSet({ frame.render_cmd_list.GetInterface() }, frame.index);
    }

    UserInterfaceApp::CompleteInitialization();
}

bool TexturedCubeApp::Animate(double, double delta_seconds)
{
    const float rotation_angle_rad = static_cast<float>(delta_seconds * 360.F / 4.F) * gfx::ConstFloat::RadPerDeg;
    hlslpp::float3x3 light_rotate_matrix = hlslpp::float3x3::rotation_axis(m_camera.GetOrientation().up, rotation_angle_rad);
    m_shader_uniforms.light_position = hlslpp::mul(m_shader_uniforms.light_position, light_rotate_matrix);
    m_camera.Rotate(m_camera.GetOrientation().up, static_cast<float>(delta_seconds * 360.F / 8.F));
    return true;
}

bool TexturedCubeApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    // Resize screen color and depth textures
    if (!UserInterfaceApp::Resize(frame_size, is_minimized))
        return false;

    m_camera.Resize(frame_size);
    return true;
}

bool TexturedCubeApp::Update()
{
    if (!UserInterfaceApp::Update())
        return false;

    // Update Model, View, Projection matrices based on camera location
    m_shader_uniforms.mvp_matrix   = hlslpp::transpose(hlslpp::mul(m_shader_uniforms.model_matrix, m_camera.GetViewProjMatrix()));
    m_shader_uniforms.eye_position = m_camera.GetOrientation().eye;
    
    return true;
}

bool TexturedCubeApp::Render()
{
    if (!UserInterfaceApp::Render())
        return false;

    // Update uniforms buffer related to current frame
    const TexturedCubeFrame& frame = GetCurrentFrame();
    const rhi::CommandQueue& render_cmd_queue = GetRenderContext().GetRenderCommandKit().GetQueue();
    frame.uniforms_buffer.SetData(render_cmd_queue, m_shader_uniforms_subresource);

    // Issue commands for cube rendering
    META_DEBUG_GROUP_VAR(s_debug_group, "Cube Rendering");
    frame.render_cmd_list.ResetWithState(m_render_state, &s_debug_group);
    frame.render_cmd_list.SetViewState(GetViewState());
    frame.render_cmd_list.SetProgramBindings(frame.program_bindings);
    frame.render_cmd_list.SetVertexBuffers(m_vertex_buffer_set);
    frame.render_cmd_list.SetIndexBuffer(m_index_buffer);
    frame.render_cmd_list.DrawIndexed(rhi::RenderPrimitive::Triangle);

    RenderOverlay(frame.render_cmd_list);

    // Execute command list on render queue and present frame to screen
    frame.render_cmd_list.Commit();
    render_cmd_queue.Execute(frame.execute_cmd_list_set);
    GetRenderContext().Present();

    return true;
}

void TexturedCubeApp::OnContextReleased(rhi::IContext& context)
{
    m_texture_sampler = {};
    m_cube_texture = {};
    m_const_buffer = {};
    m_index_buffer = {};
    m_vertex_buffer_set = {};
    m_render_state = {};

    UserInterfaceApp::OnContextReleased(context);
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::TexturedCubeApp().Run({ argc, argv });
}
