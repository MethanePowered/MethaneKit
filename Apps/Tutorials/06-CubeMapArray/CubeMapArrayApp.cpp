/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: CubeMapArrayApp.cpp
Tutorial demonstrating textured cube rendering with Methane graphics API

******************************************************************************/

#include "CubeMapArrayApp.h"

#include <Methane/Samples/AppSettings.hpp>
#include <Methane/Graphics/CubeMesh.hpp>
#include <Methane/Data/TimeAnimation.h>

#include <magic_enum.hpp>

namespace Methane::Tutorials
{

namespace gui = Methane::UserInterface;

struct CubeVertex
{
    gfx::Mesh::Position position;

    inline static const gfx::Mesh::VertexLayout layout{
        gfx::Mesh::VertexField::Position,
    };
};

CubeMapArrayApp::CubeMapArrayApp()
    : UserInterfaceApp(
        Samples::GetGraphicsAppSettings("Methane Cube Map Array",
                                        Samples::g_default_app_options_color_only_and_anim), {},
                                        "Methane tutorial of cube-map array texturing")
    , m_model_matrix(hlslpp::mul(hlslpp::float4x4::scale(15.F), hlslpp::float4x4::rotation_z(gfx::ConstFloat::Pi)))
{
    m_camera.ResetOrientation({ { 13.0F, 13.0F, -13.0F }, { 0.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } });

    // Setup animations
    GetAnimations().emplace_back(std::make_shared<Data::TimeAnimation>(std::bind(&CubeMapArrayApp::Animate, this, std::placeholders::_1, std::placeholders::_2)));
}

CubeMapArrayApp::~CubeMapArrayApp()
{
    // Wait for GPU rendering is completed to release resources
    WaitForRenderComplete();
}

void CubeMapArrayApp::Init()
{
    UserInterfaceApp::Init();

    gfx::CommandQueue& render_cmd_queue = GetRenderContext().GetRenderCommandKit().GetQueue();
    m_camera.Resize(GetRenderContext().GetSettings().frame_size);

    // Create vertex buffer for cube mesh with counter-clockwise vertex order for non-reflected cube-texture visualization
    const gfx::CubeMesh<CubeVertex> cube_mesh(CubeVertex::layout);
    const Data::Size vertex_data_size  = cube_mesh.GetVertexDataSize();
    const Data::Size vertex_size       = cube_mesh.GetVertexSize();
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

    // Create render state with program
    m_render_state_ptr = gfx::RenderState::Create(GetRenderContext(),
        gfx::RenderState::Settings
        {
            gfx::Program::Create(GetRenderContext(),
                gfx::Program::Settings
                {
                    gfx::Program::Shaders
                    {
                        gfx::Shader::CreateVertex(GetRenderContext(), { Data::ShaderProvider::Get(), { "CubeMapArray", "CubeVS" } }),
                        gfx::Shader::CreatePixel( GetRenderContext(), { Data::ShaderProvider::Get(), { "CubeMapArray", "CubePS" } }),
                    },
                    gfx::Program::InputBufferLayouts
                    {
                        gfx::Program::InputBufferLayout
                        {
                            gfx::Program::InputBufferLayout::ArgumentSemantics { cube_mesh.GetVertexLayout().GetSemantics() }
                        }
                    },
                    gfx::Program::ArgumentAccessors
                    {
                        { { gfx::Shader::Type::All,   "g_uniforms"  }, gfx::Program::ArgumentAccessor::Type::FrameConstant },
                        { { gfx::Shader::Type::Pixel, "g_constants" }, gfx::Program::ArgumentAccessor::Type::Constant },
                        { { gfx::Shader::Type::Pixel, "g_texture"   }, gfx::Program::ArgumentAccessor::Type::Constant },
                        { { gfx::Shader::Type::Pixel, "g_sampler"   }, gfx::Program::ArgumentAccessor::Type::Constant },
                    },
                    GetScreenRenderPattern().GetAttachmentFormats()
                }
            ),
            GetScreenRenderPatternPtr()
        }
    );
    m_render_state_ptr->GetSettings().program_ptr->SetName("Textured Phong Lighting");
    m_render_state_ptr->SetName("Final FB Render Pipeline State");

    // Load cube-map texture image from file
    using namespace magic_enum::bitwise_operators;
    m_cube_map_array_texture_ptr = gfx::Texture::CreateRenderTarget(GetRenderContext(), gfx::Texture::Settings::Cube(
        640U, 1U, gfx::PixelFormat::RGBA8Unorm, false, gfx::Texture::Usage::RenderTarget | gfx::Texture::Usage::ShaderRead));

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
    for(CubeMapArrayFrame& frame : GetFrames())
    {
        // Create uniforms buffer with volatile parameters for frame rendering
        frame.uniforms_buffer_ptr = gfx::Buffer::CreateConstantBuffer(GetRenderContext(), uniforms_data_size, false, true);
        frame.uniforms_buffer_ptr->SetName(IndexedName("Uniforms Buffer", frame.index));

        // Configure program resource bindings
        frame.program_bindings_ptr = gfx::ProgramBindings::Create(m_render_state_ptr->GetSettings().program_ptr, {
            { { gfx::Shader::Type::All,   "g_uniforms"  }, { { *frame.uniforms_buffer_ptr    } } },
            { { gfx::Shader::Type::Pixel, "g_texture"   }, { { *m_cube_map_array_texture_ptr } } },
            { { gfx::Shader::Type::Pixel, "g_sampler"   }, { { *m_texture_sampler_ptr        } } },
        }, frame.index);
        frame.program_bindings_ptr->SetName(IndexedName("Cube Bindings", frame.index));
        
        // Create command list for rendering
        frame.render_cmd_list_ptr = gfx::RenderCommandList::Create(GetRenderContext().GetRenderCommandKit().GetQueue(), *frame.screen_pass_ptr);
        frame.render_cmd_list_ptr->SetName(IndexedName("Cube Rendering", frame.index));
        frame.execute_cmd_list_set_ptr = gfx::CommandListSet::Create({ *frame.render_cmd_list_ptr }, frame.index);
    }
    
    // Encode face texture rendering commands before resources upload in complete initialization
    const Ptr<gfx::CommandListSet> face_render_cmd_list_set_ptr = RenderFaceTextures(*m_cube_map_array_texture_ptr);

    // Upload all resources, including font texture and text mesh buffers required for rendering
    UserInterfaceApp::CompleteInitialization();
    
    // Execute face texture rendering commands when all resources are uploaded and ready for text rendering
    render_cmd_queue.Execute(*face_render_cmd_list_set_ptr);
    GetRenderContext().WaitForGpu(gfx::Context::WaitFor::RenderComplete);
}

Ptr<gfx::CommandListSet> CubeMapArrayApp::RenderFaceTextures(gfx::Texture& rt_texture)
{
    struct Face
    {
        std::u32string label;
        gfx::Color4F   color;
        Ptr<gfx::RenderCommandList> render_cmd_list_ptr;
    };

    std::array<Face, 6> faces = {{
        { U"X+", gfx::Color4F(0.84F, 0.19F, 0.17F, 1.F) }, // red       rgb(215 48 44)
        { U"X-", gfx::Color4F(0.94F, 0.42F, 0.07F, 1.F) }, // orange    rgb(239 106 18)
        { U"Y+", gfx::Color4F(0.35F, 0.69F, 0.24F, 1.F) }, // green     rgb(89 176 60)
        { U"Y-", gfx::Color4F(0.12F, 0.62F, 0.47F, 1.F) }, // turquoise rgb(31 158 120)
        { U"Z+", gfx::Color4F(0.20F, 0.36F, 0.66F, 1.F) }, // blue      rgb(51 93 169)
        { U"Z-", gfx::Color4F(0.49F, 0.31F, 0.64F, 1.F) }  // purple    rgb(124 80 164)
    }};

    const gfx::Texture::Settings& rt_texture_settings = rt_texture.GetSettings();
    gfx::RenderPattern::Settings render_pattern_settings{
        {
            gfx::RenderPattern::ColorAttachment(
                0U, rt_texture_settings.pixel_format, 1U,
                gfx::RenderPattern::ColorAttachment::LoadAction::Clear,
                gfx::RenderPattern::ColorAttachment::StoreAction::Store)
        },
        std::nullopt, // No depth attachment
        std::nullopt, // No stencil attachment
        gfx::RenderPass::Access::ShaderResources,
        false // intermediate render pass
    };
    
    gui::Font& face_font = gui::Font::Library::Get().GetFont(GetFontProvider(), gui::Font::Settings{
        { "Face Labels",  "Fonts/RobotoMono/RobotoMono-Regular.ttf", 164U }, 96U, U"XYZ+-0123456789"
    });

    gui::Text::SettingsUtf32 face_text_settings
    {
        {}, {},
        gui::UnitRect
        {
            gui::Units::Pixels,
            gfx::Point2I(),
            rt_texture_settings.dimensions.AsRectSize()
        },
        gui::Text::Layout
        {
            gui::Text::Wrap::None,
            gui::Text::HorizontalAlignment::Center,
            gui::Text::VerticalAlignment::Center,
        },
        gfx::Color4F(1.F, 1.F, 1.F, 1.F),
        false
    };

    gfx::CommandQueue& render_cmd_queue = GetRenderContext().GetRenderCommandKit().GetQueue();
    Refs<gfx::CommandList> face_render_cmd_lists;

    META_DEBUG_GROUP_CREATE_VAR(s_debug_group_ptr, "Texture Faces Rendering");
    const gfx::SubResource::Count rt_subresource_count = rt_texture.GetSubresourceCount();
    for(uint32_t face_index = 0U; face_index < rt_subresource_count.GetDepth(); ++face_index)
    {
        Face& face = faces[face_index];
        render_pattern_settings.color_attachments[0].clear_color = face.color;
        face_text_settings.name = gui::Font::ConvertUtf32To8(face.label) + " Face Label";
        face_text_settings.text = face.label;

        const Ptr<gfx::RenderPattern> face_render_pattern_ptr = gfx::RenderPattern::Create(GetRenderContext(), render_pattern_settings);
        const Ptr<gfx::RenderPass>    face_render_pass_ptr    = gfx::RenderPass::Create(*face_render_pattern_ptr, {
            { gfx::Texture::Location(rt_texture, gfx::SubResource::Index(face_index)) },
            rt_texture_settings.dimensions.AsRectSize()
        });
        face.render_cmd_list_ptr = gfx::RenderCommandList::Create(render_cmd_queue, *face_render_pass_ptr);
        face.render_cmd_list_ptr->SetName(IndexedName("Render Texture Face", face_index));

        gui::Text face_text(GetUIContext(), *face_render_pattern_ptr, face_font, face_text_settings);
        face_text.Update(rt_texture_settings.dimensions.AsRectSize());
        face_text.Draw(*face.render_cmd_list_ptr, s_debug_group_ptr.get());
        face.render_cmd_list_ptr->Commit();

        face_render_cmd_lists.emplace_back(*face.render_cmd_list_ptr);
    }

    return gfx::CommandListSet::Create(face_render_cmd_lists);
}

bool CubeMapArrayApp::Animate(double, double delta_seconds)
{
    m_camera.Rotate(m_camera.GetOrientation().up, static_cast<float>(delta_seconds * 360.F / 8.F));
    return true;
}

bool CubeMapArrayApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    // Resize screen color and depth textures
    if (!UserInterfaceApp::Resize(frame_size, is_minimized))
        return false;

    m_camera.Resize(frame_size);
    return true;
}

bool CubeMapArrayApp::Update()
{
    if (!UserInterfaceApp::Update())
        return false;

    // Update Model, View, Projection matrices based on camera location
    m_shader_uniforms.mvp_matrix = hlslpp::transpose(hlslpp::mul(m_model_matrix, m_camera.GetViewProjMatrix()));
    return true;
}

bool CubeMapArrayApp::Render()
{
    if (!UserInterfaceApp::Render())
        return false;

    // Update uniforms buffer related to current frame
    const CubeMapArrayFrame& frame = GetCurrentFrame();
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

void CubeMapArrayApp::OnContextReleased(gfx::Context& context)
{
    m_texture_sampler_ptr.reset();
    m_cube_map_array_texture_ptr.reset();
    m_index_buffer_ptr.reset();
    m_vertex_buffer_set_ptr.reset();
    m_render_state_ptr.reset();

    UserInterfaceApp::OnContextReleased(context);
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::CubeMapArrayApp().Run({ argc, argv });
}
