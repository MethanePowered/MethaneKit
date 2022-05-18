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
#include "TextureLabeler.h"

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

    // Create cube mesh
    gfx::CubeMesh<CubeVertex> cube_mesh(CubeVertex::layout);

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

    // Create cube mesh buffer resources
    m_cube_buffers_ptr = std::make_unique<TexturedMeshBuffers>(render_cmd_queue, std::move(cube_mesh), "Cube");

    // Create cube-map render target texture
    using namespace magic_enum::bitwise_operators;
    m_cube_buffers_ptr->SetTexture(
        gfx::Texture::CreateRenderTarget(GetRenderContext(),
            gfx::Texture::Settings::Cube(640U, 1U, gfx::PixelFormat::RGBA8Unorm, false,
                                         gfx::Texture::Usage::RenderTarget | gfx::Texture::Usage::ShaderRead)));

    // Create sampler for image texture
    m_texture_sampler_ptr = gfx::Sampler::Create(GetRenderContext(),
        gfx::Sampler::Settings
        {
            gfx::Sampler::Filter  { gfx::Sampler::Filter::MinMag::Linear },
            gfx::Sampler::Address { gfx::Sampler::Address::Mode::ClampToEdge }
        }
    );

    // Create frame buffer resources
    const auto uniforms_data_size = m_cube_buffers_ptr->GetUniformsBufferSize();
    for(CubeMapArrayFrame& frame : GetFrames())
    {
        // Create uniforms buffer with volatile parameters for frame rendering
        frame.uniforms_buffer_ptr = gfx::Buffer::CreateConstantBuffer(GetRenderContext(), uniforms_data_size, false, true);
        frame.uniforms_buffer_ptr->SetName(IndexedName("Uniforms Buffer", frame.index));

        // Configure program resource bindings
        frame.program_bindings_ptr = gfx::ProgramBindings::Create(m_render_state_ptr->GetSettings().program_ptr, {
            { { gfx::Shader::Type::All,   "g_uniforms"  }, { { *frame.uniforms_buffer_ptr       } } },
            { { gfx::Shader::Type::Pixel, "g_texture"   }, { { m_cube_buffers_ptr->GetTexture() } } },
            { { gfx::Shader::Type::Pixel, "g_sampler"   }, { { *m_texture_sampler_ptr           } } },
        }, frame.index);
        frame.program_bindings_ptr->SetName(IndexedName("Cube Bindings", frame.index));
        
        // Create command list for rendering
        frame.render_cmd_list_ptr = gfx::RenderCommandList::Create(GetRenderContext().GetRenderCommandKit().GetQueue(), *frame.screen_pass_ptr);
        frame.render_cmd_list_ptr->SetName(IndexedName("Cube Rendering", frame.index));
        frame.execute_cmd_list_set_ptr = gfx::CommandListSet::Create({ *frame.render_cmd_list_ptr }, frame.index);
    }
    
    // Create all resources for texture labels rendering before resources upload in UserInterfaceApp::CompleteInitialization()
    TextureLabeler texture_labeler(GetUIContext(), GetFontProvider(), m_cube_buffers_ptr->GetTexture());

    // Upload all resources, including font texture and text mesh buffers required for rendering
    UserInterfaceApp::CompleteInitialization();
    
    // Encode and execute texture labels rendering commands when all resources are uploaded and ready on GPU
    texture_labeler.Render();
    GetRenderContext().WaitForGpu(gfx::Context::WaitFor::RenderComplete);
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
    m_cube_buffers_ptr->SetFinalPassUniforms(hlslpp::Uniforms{
        hlslpp::transpose(hlslpp::mul(m_model_matrix, m_camera.GetViewProjMatrix()))
    });
    return true;
}

bool CubeMapArrayApp::Render()
{
    if (!UserInterfaceApp::Render())
        return false;

    // Update uniforms buffer related to current frame
    const CubeMapArrayFrame& frame = GetCurrentFrame();
    gfx::CommandQueue& render_cmd_queue = GetRenderContext().GetRenderCommandKit().GetQueue();
    frame.uniforms_buffer_ptr->SetData(m_cube_buffers_ptr->GetFinalPassUniformsSubresources(), render_cmd_queue);

    // Render cube
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Cube Rendering");
    frame.render_cmd_list_ptr->ResetWithState(*m_render_state_ptr, s_debug_group.get());
    frame.render_cmd_list_ptr->SetViewState(GetViewState());
    m_cube_buffers_ptr->Draw(*frame.render_cmd_list_ptr, *frame.program_bindings_ptr);

    RenderOverlay(*frame.render_cmd_list_ptr);

    frame.render_cmd_list_ptr->Commit();

    // Execute command list on render queue and present frame to screen
    render_cmd_queue.Execute(*frame.execute_cmd_list_set_ptr);
    GetRenderContext().Present();

    return true;
}

void CubeMapArrayApp::OnContextReleased(gfx::Context& context)
{
    m_cube_buffers_ptr.reset();
    m_texture_sampler_ptr.reset();
    m_render_state_ptr.reset();

    UserInterfaceApp::OnContextReleased(context);
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::CubeMapArrayApp().Run({ argc, argv });
}
