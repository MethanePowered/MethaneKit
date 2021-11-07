/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: HelloCubeUniformsApp.cpp
Tutorial demonstrating colored cube rendering using uniform buffers

******************************************************************************/

#include <Methane/Kit.h>
#include <Methane/Graphics/App.hpp>
#include <Methane/Graphics/CubeMesh.hpp>
#include <Methane/Samples/AppSettings.hpp>
#include <Methane/Data/TimeAnimation.h>

namespace hlslpp // NOSONAR
{
#pragma pack(push, 16)
#include "Shaders/HelloCubeUniforms.h" // NOSONAR
#pragma pack(pop)
}

using namespace Methane;
using namespace Methane::Graphics;

struct HelloCubeUniformsFrame final : AppFrame
{
    Ptr<Buffer>            uniforms_buffer_ptr;
    Ptr<ProgramBindings>   program_bindings_ptr;
    Ptr<RenderCommandList> render_cmd_list_ptr;
    Ptr<CommandListSet>    execute_cmd_list_set_ptr;

    using AppFrame::AppFrame;
};

using GraphicsApp = App<HelloCubeUniformsFrame>;
class HelloCubeUniformsApp final : public GraphicsApp // NOSONAR
{
private:
    struct CubeVertex
    {
        Mesh::Position position;
        Mesh::Color    color;

        inline static const Mesh::VertexLayout layout{
            Mesh::VertexField::Position,
            Mesh::VertexField::Color
        };
    };

    const CubeMesh<CubeVertex> m_cube_mesh{ CubeVertex::layout };
    const hlslpp::float4x4     m_model_matrix = hlslpp::float4x4::scale(15.F);
    hlslpp::Uniforms           m_shader_uniforms { };
    Camera                     m_camera;

    Ptr<RenderState> m_render_state_ptr;
    Ptr<Buffer>      m_index_buffer_ptr;
    Ptr<BufferSet>   m_vertex_buffer_set_ptr;

    const Resource::SubResources m_shader_uniforms_subresources{
        { reinterpret_cast<Data::ConstRawPtr>(&m_shader_uniforms), sizeof(hlslpp::Uniforms) } // NOSONAR
    };

public:
    HelloCubeUniformsApp()
        : GraphicsApp(
            []() {
                Graphics::AppSettings settings = Samples::GetGraphicsAppSettings("Methane Hello Cube Uniforms", Samples::g_default_app_options_color_only_and_anim);
                settings.graphics_app.SetScreenPassAccess(RenderPass::Access::ShaderResources);
                return settings;
            }(),
            "Tutorial demonstrating colored rotating cube rendering using uniform buffers with Methane Kit.")
    {
        m_camera.ResetOrientation({ { 13.0F, 13.0F, 13.0F }, { 0.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } });

        // Setup camera rotation animation
        GetAnimations().emplace_back(std::make_shared<Data::TimeAnimation>(
            [this](double, double delta_seconds)
            {
                m_camera.Rotate(m_camera.GetOrientation().up, static_cast<float>(delta_seconds * 360.F / 8.F));
                return true;
            }));
    }

    ~HelloCubeUniformsApp() override
    {
        // Wait for GPU rendering is completed to release resources
        WaitForRenderComplete();
    }

    void Init() override
    {
        GraphicsApp::Init();

        m_camera.Resize(GetRenderContext().GetSettings().frame_size);

        // Create render state with program
        m_render_state_ptr = RenderState::Create(GetRenderContext(),
            RenderState::Settings
            {
                Program::Create(GetRenderContext(),
                    Program::Settings
                    {
                        Program::Shaders
                        {
                            Shader::CreateVertex(GetRenderContext(), { Data::ShaderProvider::Get(), { "HelloCube", "CubeVS" } }),
                            Shader::CreatePixel( GetRenderContext(), { Data::ShaderProvider::Get(), { "HelloCube", "CubePS" } }),
                        },
                        Program::InputBufferLayouts
                        {
                            Program::InputBufferLayout
                            {
                                Program::InputBufferLayout::ArgumentSemantics { "POSITION" , "COLOR" }
                            }
                        },
                        Program::ArgumentAccessors
                        {
                            { { Shader::Type::Vertex, "g_uniforms" }, Program::ArgumentAccessor::Type::FrameConstant }
                        },
                        GetScreenRenderPattern().GetAttachmentFormats()
                    }
                ),
                GetScreenRenderPatternPtr()
            }
        );
        m_render_state_ptr->GetSettings().program_ptr->SetName("Colored Cube Shading");
        m_render_state_ptr->SetName("Colored Cube Pipeline State");

        // Create index buffer for cube mesh
        m_index_buffer_ptr = Buffer::CreateIndexBuffer(GetRenderContext(), m_cube_mesh.GetIndexDataSize(), GetIndexFormat(m_cube_mesh.GetIndex(0)));
        m_index_buffer_ptr->SetName("Cube Index Buffer");
        m_index_buffer_ptr->SetData({ { reinterpret_cast<Data::ConstRawPtr>(m_cube_mesh.GetIndices().data()), m_cube_mesh.GetIndexDataSize() } }); // NOSONAR

        // Create vertex buffers
        Ptr<Buffer> vertex_buffer_ptr = Buffer::CreateVertexBuffer(GetRenderContext(), m_cube_mesh.GetVertexDataSize(), m_cube_mesh.GetVertexSize());
        vertex_buffer_ptr->SetName("Cube Vertex Buffer");
        vertex_buffer_ptr->SetData({ { reinterpret_cast<Data::ConstRawPtr>(m_cube_mesh.GetVertices().data()), m_cube_mesh.GetVertexDataSize() } }); // NOSONAR
        m_vertex_buffer_set_ptr = BufferSet::CreateVertexBuffers({ *vertex_buffer_ptr });

        // Create per-frame command lists
        const auto uniforms_data_size = static_cast<Data::Size>(sizeof(m_shader_uniforms));
        for(HelloCubeUniformsFrame& frame : GetFrames())
        {
            // Create uniforms buffer with volatile parameters for frame rendering
            frame.uniforms_buffer_ptr = Buffer::CreateConstantBuffer(GetRenderContext(), uniforms_data_size, false, true);
            frame.uniforms_buffer_ptr->SetName(IndexedName("Uniforms Buffer", frame.index));

            // Configure program resource bindings
            frame.program_bindings_ptr = ProgramBindings::Create(m_render_state_ptr->GetSettings().program_ptr, {
                { { Shader::Type::Vertex, "g_uniforms"  }, { { *frame.uniforms_buffer_ptr } } }
            }, frame.index);

            // Create command list for rendering
            frame.render_cmd_list_ptr = RenderCommandList::Create(GetRenderContext().GetRenderCommandKit().GetQueue(), *frame.screen_pass_ptr);
            frame.render_cmd_list_ptr->SetName(IndexedName("Cube Rendering", frame.index));
            frame.execute_cmd_list_set_ptr = CommandListSet::Create({ *frame.render_cmd_list_ptr });
        }

        GraphicsApp::CompleteInitialization();
    }

    bool Resize(const FrameSize& frame_size, bool is_minimized) override
    {
        // Resize screen color and depth textures
        if (!GraphicsApp::Resize(frame_size, is_minimized))
            return false;

        m_camera.Resize(frame_size);
        return true;
    }

    bool Update() override
    {
        if (!GraphicsApp::Update())
            return false;

        // Update uniforms buffer with camera Model-View-Projection matrix
        m_shader_uniforms.mvp_matrix = hlslpp::transpose(hlslpp::mul(m_model_matrix, m_camera.GetViewProjMatrix()));
        return true;
    }

    bool Render() override
    {
        if (!GraphicsApp::Render())
            return false;

        const HelloCubeUniformsFrame& frame = GetCurrentFrame();
        frame.uniforms_buffer_ptr->SetData(m_shader_uniforms_subresources);

        // Issue commands for cube rendering
        META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Cube Rendering");
        frame.render_cmd_list_ptr->ResetWithState(*m_render_state_ptr, s_debug_group.get());
        frame.render_cmd_list_ptr->SetViewState(GetViewState());
        frame.render_cmd_list_ptr->SetProgramBindings(*frame.program_bindings_ptr);
        frame.render_cmd_list_ptr->SetVertexBuffers(*m_vertex_buffer_set_ptr);
        frame.render_cmd_list_ptr->SetIndexBuffer(*m_index_buffer_ptr);
        frame.render_cmd_list_ptr->DrawIndexed(RenderCommandList::Primitive::Triangle);
        frame.render_cmd_list_ptr->Commit();

        // Execute command list on render queue and present frame to screen
        GetRenderContext().GetRenderCommandKit().GetQueue().Execute(*frame.execute_cmd_list_set_ptr);
        GetRenderContext().Present();

        return true;
    }

    void OnContextReleased(Context& context) override
    {
        m_vertex_buffer_set_ptr.reset();
        m_index_buffer_ptr.reset();
        m_render_state_ptr.reset();

        GraphicsApp::OnContextReleased(context);
    }
};

int main(int argc, const char* argv[])
{
    return HelloCubeUniformsApp().Run({ argc, argv });
}
