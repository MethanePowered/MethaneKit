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

#include <Methane/Samples/AppSettings.hpp>
#include <Methane/Graphics/Mesh/CubeMesh.hpp>
#include <Methane/Data/TimeAnimation.h>

#include <cml/mathlib/mathlib.h>

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
        Samples::GetGraphicsAppSettings("Methane Textured Cube"), {},
        "Methane tutorial of textured cube rendering")
    , m_shader_constants(                               // Shader constants:
        {                                               // ================
            gfx::Color4f(1.f, 1.f, 0.74f, 1.f),         // - light_color
            700.f,                                      // - light_power
            0.04f,                                      // - light_ambient_factor
            30.f                                        // - light_specular_factor
        })
    , m_cube_scale(15.f)
{
    m_shader_uniforms.light_position = gfx::Vector3f(0.f, 20.f, -25.f);
    m_camera.ResetOrientation({ { 13.0f, 13.0f, -13.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } });

    // Setup animations
    GetAnimations().emplace_back(std::make_shared<Data::TimeAnimation>(std::bind(&TexturedCubeApp::Animate, this, std::placeholders::_1, std::placeholders::_2)));
}

TexturedCubeApp::~TexturedCubeApp()
{
    // Wait for GPU rendering is completed to release resources
    GetRenderContext().WaitForGpu(gfx::RenderContext::WaitFor::RenderComplete);
}

void TexturedCubeApp::Init()
{
    UserInterfaceApp::Init();

    const gfx::RenderContext::Settings& context_settings = GetRenderContext().GetSettings();
    m_camera.Resize({
        static_cast<float>(context_settings.frame_size.width),
        static_cast<float>(context_settings.frame_size.height)
    });

    const gfx::CubeMesh<CubeVertex> cube_mesh(CubeVertex::layout);

    // Create render state with program
    gfx::RenderState::Settings state_settings;
    state_settings.sp_program = gfx::Program::Create(GetRenderContext(),
        gfx::Program::Settings
        {
            gfx::Program::Shaders
            {
                gfx::Shader::CreateVertex(GetRenderContext(), { Data::ShaderProvider::Get(), { "Cube", "CubeVS" } }),
                gfx::Shader::CreatePixel( GetRenderContext(), { Data::ShaderProvider::Get(), { "Cube", "CubePS" } }),
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
                { { gfx::Shader::Type::All,   "g_uniforms"  }, gfx::Program::Argument::Modifiers::None     },
                { { gfx::Shader::Type::Pixel, "g_constants" }, gfx::Program::Argument::Modifiers::Constant },
                { { gfx::Shader::Type::Pixel, "g_texture"   }, gfx::Program::Argument::Modifiers::Constant },
                { { gfx::Shader::Type::Pixel, "g_sampler"   }, gfx::Program::Argument::Modifiers::Constant },
            },
            gfx::PixelFormats
            {
                context_settings.color_format
            },
            context_settings.depth_stencil_format
        }
    );
    state_settings.sp_program->SetName("Textured Phong Lighting");
    state_settings.depth.enabled = true;
    m_sp_render_state = gfx::RenderState::Create(GetRenderContext(), state_settings);
    m_sp_render_state->SetName("Final FB Render Pipeline State");

    // Load texture image from file
    const gfx::ImageLoader::Options::Mask image_options = gfx::ImageLoader::Options::Mipmapped
                                                        | gfx::ImageLoader::Options::SrgbColorSpace;
    m_sp_cube_texture = GetImageLoader().LoadImageToTexture2D(GetRenderContext(), "Textures/MethaneBubbles.jpg", image_options);
    m_sp_cube_texture->SetName("Cube Texture 2D Image");

    // Create sampler for image texture
    m_sp_texture_sampler = gfx::Sampler::Create(GetRenderContext(),
        gfx::Sampler::Settings
        {
            gfx::Sampler::Filter  { gfx::Sampler::Filter::MinMag::Linear },
            gfx::Sampler::Address { gfx::Sampler::Address::Mode::ClampToEdge }
        }
    );

    const Data::Size constants_data_size = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(m_shader_constants)));
    const Data::Size uniforms_data_size  = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(m_shader_uniforms)));

    // Create vertex buffer for cube mesh
    const Data::Size vertex_data_size = static_cast<Data::Size>(cube_mesh.GetVertexDataSize());
    const Data::Size vertex_size      = static_cast<Data::Size>(cube_mesh.GetVertexSize());
    Ptr<gfx::Buffer> sp_vertex_buffer = gfx::Buffer::CreateVertexBuffer(GetRenderContext(), vertex_data_size, vertex_size);
    sp_vertex_buffer->SetName("Cube Vertex Buffer");
    sp_vertex_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(cube_mesh.GetVertices().data()), vertex_data_size } });
    m_sp_vertex_buffer_set = gfx::BufferSet::CreateVertexBuffers({ *sp_vertex_buffer });

    // Create index buffer for cube mesh
    const Data::Size index_data_size = static_cast<Data::Size>(cube_mesh.GetIndexDataSize());
    m_sp_index_buffer  = gfx::Buffer::CreateIndexBuffer(GetRenderContext(), index_data_size, gfx::GetIndexFormat(cube_mesh.GetIndex(0)));
    m_sp_index_buffer->SetName("Cube Index Buffer");
    m_sp_index_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(cube_mesh.GetIndices().data()), index_data_size } });

    // Create constants buffer for frame rendering
    m_sp_const_buffer = gfx::Buffer::CreateConstantBuffer(GetRenderContext(), constants_data_size);
    m_sp_const_buffer->SetName("Constants Buffer");
    m_sp_const_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&m_shader_constants), sizeof(m_shader_constants) } });

    // Create frame buffer data
    for(TexturedCubeFrame& frame : GetFrames())
    {
        // Create uniforms buffer with volatile parameters for frame rendering
        frame.sp_uniforms_buffer = gfx::Buffer::CreateVolatileBuffer(GetRenderContext(), uniforms_data_size);
        frame.sp_uniforms_buffer->SetName(IndexedName("Uniforms Buffer", frame.index));

        // Configure program resource bindings
        frame.sp_program_bindings = gfx::ProgramBindings::Create(state_settings.sp_program, {
            { { gfx::Shader::Type::All,   "g_uniforms"  }, { { frame.sp_uniforms_buffer } } },
            { { gfx::Shader::Type::Pixel, "g_constants" }, { { m_sp_const_buffer        } } },
            { { gfx::Shader::Type::Pixel, "g_texture"   }, { { m_sp_cube_texture        } } },
            { { gfx::Shader::Type::Pixel, "g_sampler"   }, { { m_sp_texture_sampler     } } },
        });
        
        // Create command list for rendering
        frame.sp_render_cmd_list = gfx::RenderCommandList::Create(GetRenderContext().GetRenderCommandQueue(), *frame.sp_screen_pass);
        frame.sp_render_cmd_list->SetName(IndexedName("Cube Rendering", frame.index));
        frame.sp_execute_cmd_list_set = gfx::CommandListSet::Create({ *frame.sp_render_cmd_list });
    }

    UserInterfaceApp::CompleteInitialization();
}

bool TexturedCubeApp::Animate(double, double delta_seconds)
{
    gfx::Matrix33f light_rotate_matrix;
    cml::matrix_rotation_axis_angle(light_rotate_matrix, m_camera.GetOrientation().up, cml::rad(360.f * delta_seconds / 4.f));
    m_shader_uniforms.light_position = m_shader_uniforms.light_position * light_rotate_matrix;
    m_camera.Rotate(m_camera.GetOrientation().up, static_cast<float>(delta_seconds * 360.f / 8.f));
    return true;
}

bool TexturedCubeApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    // Resize screen color and depth textures
    if (!UserInterfaceApp::Resize(frame_size, is_minimized))
        return false;

    m_camera.Resize({
        static_cast<float>(frame_size.width),
        static_cast<float>(frame_size.height)
    });

    return true;
}

bool TexturedCubeApp::Update()
{
    if (!UserInterfaceApp::Update())
        return false;

    // Update Model, View, Projection matrices based on camera location
    gfx::Matrix44f model_matrix;
    cml::matrix_uniform_scale(model_matrix, m_cube_scale);

    m_shader_uniforms.mvp_matrix     = model_matrix * m_camera.GetViewProjMatrix();
    m_shader_uniforms.model_matrix   = model_matrix;
    m_shader_uniforms.eye_position   = gfx::Vector4f(m_camera.GetOrientation().eye, 1.f);
    
    return true;
}

bool TexturedCubeApp::Render()
{
    if (!UserInterfaceApp::Render())
        return false;

    // Update uniforms buffer related to current frame
    TexturedCubeFrame& frame = GetCurrentFrame();
    frame.sp_uniforms_buffer->SetData(m_shader_uniforms_subresources);

    // Issue commands for cube rendering
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Cube Rendering");
    frame.sp_render_cmd_list->Reset(m_sp_render_state, s_debug_group.get());
    frame.sp_render_cmd_list->SetViewState(GetViewState());
    frame.sp_render_cmd_list->SetProgramBindings(*frame.sp_program_bindings);
    frame.sp_render_cmd_list->SetVertexBuffers(*m_sp_vertex_buffer_set);
    frame.sp_render_cmd_list->DrawIndexed(gfx::RenderCommandList::Primitive::Triangle, *m_sp_index_buffer);

    RenderOverlay(*frame.sp_render_cmd_list);

    // Commit command list with present flag
    frame.sp_render_cmd_list->Commit();

    // Execute command list on render queue and present frame to screen
    GetRenderContext().GetRenderCommandQueue().Execute(*frame.sp_execute_cmd_list_set);
    GetRenderContext().Present();

    return true;
}

void TexturedCubeApp::OnContextReleased(gfx::Context& context)
{
    m_sp_texture_sampler.reset();
    m_sp_cube_texture.reset();
    m_sp_const_buffer.reset();
    m_sp_index_buffer.reset();
    m_sp_vertex_buffer_set.reset();
    m_sp_render_state.reset();

    UserInterfaceApp::OnContextReleased(context);
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::TexturedCubeApp().Run({ argc, argv });
}
