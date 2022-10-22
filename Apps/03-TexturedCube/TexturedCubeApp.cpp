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

#include <Methane/Samples/AppSettings.hpp>
#include <Methane/Graphics/CubeMesh.hpp>
#include <Methane/Data/TimeAnimation.h>

#include <magic_enum.hpp>

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
        GetGraphicsTutorialAppSettings("Methane Textured Cube", g_default_app_options_color_only_and_anim),
        GetUserInterfaceTutorialAppSettings(g_default_app_options_color_only_and_anim),
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

    gfx::CommandQueue& render_cmd_queue = GetRenderContext().GetRenderCommandKit().GetQueue();
    m_camera.Resize(GetRenderContext().GetSettings().frame_size);

    // Create vertex buffer for cube mesh
    const gfx::CubeMesh<CubeVertex> cube_mesh(CubeVertex::layout);
    const Data::Size vertex_data_size   = cube_mesh.GetVertexDataSize();
    const Data::Size vertex_size        = cube_mesh.GetVertexSize();
    Ptr<gfx::Buffer> vertex_buffer_ptr = gfx::Buffer::CreateVertexBuffer(GetRenderContext(), vertex_data_size, vertex_size);
    vertex_buffer_ptr->SetName("Cube Vertex Buffer");
    vertex_buffer_ptr->SetData(
        { { reinterpret_cast<Data::ConstRawPtr>(cube_mesh.GetVertices().data()), vertex_data_size } }, // NOSONAR
        render_cmd_queue
    );
    m_vertex_buffer_set_ptr = gfx::BufferSet::CreateVertexBuffers({ *vertex_buffer_ptr });

    // Create index buffer for cube mesh
    const Data::Size index_data_size = cube_mesh.GetIndexDataSize();
    m_index_buffer_ptr = gfx::Buffer::CreateIndexBuffer(GetRenderContext(), index_data_size, gfx::GetIndexFormat(cube_mesh.GetIndex(0)));
    m_index_buffer_ptr->SetName("Cube Index Buffer");
    m_index_buffer_ptr->SetData(
        { { reinterpret_cast<Data::ConstRawPtr>(cube_mesh.GetIndices().data()), index_data_size } }, // NOSONAR
        render_cmd_queue
    );

    // Create constants buffer for frame rendering
    const auto constants_data_size = static_cast<Data::Size>(sizeof(m_shader_constants));
    m_const_buffer_ptr = gfx::Buffer::CreateConstantBuffer(GetRenderContext(), constants_data_size);
    m_const_buffer_ptr->SetName("Constants Buffer");
    m_const_buffer_ptr->SetData(
        { { reinterpret_cast<Data::ConstRawPtr>(&m_shader_constants), constants_data_size } }, // NOSONAR
        render_cmd_queue
    );

    // Create render state with program
    m_render_state_ptr = gfx::RenderState::Create(GetRenderContext(),
        gfx::RenderState::Settings
        {
            gfx::IProgram::Create(GetRenderContext(),
                gfx::IProgram::Settings
                {
                    gfx::IProgram::Shaders
                    {
                        gfx::IShader::CreateVertex(GetRenderContext(), { Data::ShaderProvider::Get(), { "TexturedCube", "CubeVS" } }),
                        gfx::IShader::CreatePixel(GetRenderContext(), { Data::ShaderProvider::Get(), { "TexturedCube", "CubePS" } }),
                    },
                    gfx::IProgram::InputBufferLayouts
                    {
                        gfx::IProgram::InputBufferLayout
                        {
                            gfx::IProgram::InputBufferLayout::ArgumentSemantics { cube_mesh.GetVertexLayout().GetSemantics() }
                        }
                    },
                    gfx::IProgram::ArgumentAccessors
                    {
                        { { gfx::ShaderType::All,   "g_uniforms"  }, gfx::IProgram::ArgumentAccessor::Type::FrameConstant },
                        { { gfx::ShaderType::Pixel, "g_constants" }, gfx::IProgram::ArgumentAccessor::Type::Constant },
                        { { gfx::ShaderType::Pixel, "g_texture"   }, gfx::IProgram::ArgumentAccessor::Type::Constant },
                        { { gfx::ShaderType::Pixel, "g_sampler"   }, gfx::IProgram::ArgumentAccessor::Type::Constant },
                    },
                    GetScreenRenderPattern().GetAttachmentFormats()
                }
            ),
            GetScreenRenderPatternPtr()
        }
    );
    m_render_state_ptr->GetSettings().program_ptr->SetName("Textured Phong Lighting");
    m_render_state_ptr->SetName("Final FB Render Pipeline State");

    // Load texture image from file
    using namespace magic_enum::bitwise_operators;
    const gfx::ImageLoader::Options image_options = gfx::ImageLoader::Options::Mipmapped
                                                  | gfx::ImageLoader::Options::SrgbColorSpace;
    m_cube_texture_ptr = GetImageLoader().LoadImageToTexture2D(render_cmd_queue, "MethaneBubbles.jpg", image_options, "Cube Face Texture");

    // Create sampler for image texture
    m_texture_sampler_ptr = gfx::Sampler::Create(GetRenderContext(),
        gfx::Sampler::Settings
        {
            gfx::Sampler::Filter  { gfx::Sampler::Filter::MinMag::Linear },
            gfx::Sampler::Address { gfx::Sampler::Address::Mode::ClampToEdge }
        }
    );

    // Create frame buffer resources
    const auto uniforms_data_size = static_cast<Data::Size>(sizeof(m_shader_uniforms));
    for(TexturedCubeFrame& frame : GetFrames())
    {
        // Create uniforms buffer with volatile parameters for frame rendering
        frame.uniforms_buffer_ptr = gfx::Buffer::CreateConstantBuffer(GetRenderContext(), uniforms_data_size, false, true);
        frame.uniforms_buffer_ptr->SetName(IndexedName("Uniforms Buffer", frame.index));

        // Configure program resource bindings
        frame.program_bindings_ptr = gfx::ProgramBindings::Create(m_render_state_ptr->GetSettings().program_ptr, {
            { { gfx::ShaderType::All,   "g_uniforms"  }, { { *frame.uniforms_buffer_ptr } } },
            { { gfx::ShaderType::Pixel, "g_constants" }, { { *m_const_buffer_ptr        } } },
            { { gfx::ShaderType::Pixel, "g_texture"   }, { { *m_cube_texture_ptr        } } },
            { { gfx::ShaderType::Pixel, "g_sampler"   }, { { *m_texture_sampler_ptr     } } },
        }, frame.index);
        frame.program_bindings_ptr->SetName(IndexedName("Cube Bindings", frame.index));
        
        // Create command list for rendering
        frame.render_cmd_list_ptr = gfx::RenderCommandList::Create(GetRenderContext().GetRenderCommandKit().GetQueue(), *frame.screen_pass_ptr);
        frame.render_cmd_list_ptr->SetName(IndexedName("Cube Rendering", frame.index));
        frame.execute_cmd_list_set_ptr = gfx::CommandListSet::Create({ *frame.render_cmd_list_ptr }, frame.index);
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
    gfx::CommandQueue& render_cmd_queue = GetRenderContext().GetRenderCommandKit().GetQueue();
    frame.uniforms_buffer_ptr->SetData(m_shader_uniforms_subresources, render_cmd_queue);

    // Issue commands for cube rendering
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Cube Rendering");
    frame.render_cmd_list_ptr->ResetWithState(*m_render_state_ptr, s_debug_group.get());
    frame.render_cmd_list_ptr->SetViewState(GetViewState());
    frame.render_cmd_list_ptr->SetProgramBindings(*frame.program_bindings_ptr);
    frame.render_cmd_list_ptr->SetVertexBuffers(*m_vertex_buffer_set_ptr);
    frame.render_cmd_list_ptr->SetIndexBuffer(*m_index_buffer_ptr);
    frame.render_cmd_list_ptr->DrawIndexed(gfx::RenderCommandList::Primitive::Triangle);

    RenderOverlay(*frame.render_cmd_list_ptr);

    frame.render_cmd_list_ptr->Commit();

    // Execute command list on render queue and present frame to screen
    render_cmd_queue.Execute(*frame.execute_cmd_list_set_ptr);
    GetRenderContext().Present();

    return true;
}

void TexturedCubeApp::OnContextReleased(gfx::IContext& context)
{
    m_texture_sampler_ptr.reset();
    m_cube_texture_ptr.reset();
    m_const_buffer_ptr.reset();
    m_index_buffer_ptr.reset();
    m_vertex_buffer_set_ptr.reset();
    m_render_state_ptr.reset();

    UserInterfaceApp::OnContextReleased(context);
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::TexturedCubeApp().Run({ argc, argv });
}
