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

FILE: HelloCubeApp.cpp
Tutorial demonstrating colored cube rendering with Methane graphics API

******************************************************************************/

#include <Methane/Kit.h>
#include <Methane/Graphics/App.hpp>
#include <Methane/Graphics/CubeMesh.hpp>
#include <Methane/Samples/AppSettings.hpp>
#include <Methane/Data/TimeAnimation.h>

using namespace Methane;
using namespace Methane::Graphics;

struct HelloCubeFrame final : AppFrame
{
    Ptr<BufferSet>         vertex_buffer_set_ptr;
    Ptr<RenderCommandList> render_cmd_list_ptr;
    Ptr<CommandListSet>    execute_cmd_list_set_ptr;

    using AppFrame::AppFrame;
};

using GraphicsApp = App<HelloCubeFrame>;
class HelloCubeApp final : public GraphicsApp // NOSONAR
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
    std::vector<CubeVertex>    m_proj_vertices;
    Camera                     m_camera;

    Ptr<RenderState> m_render_state_ptr;
    Ptr<Buffer>      m_index_buffer_ptr;

public:
    HelloCubeApp()
        : GraphicsApp(
            []() {
                Graphics::AppSettings settings = Samples::GetGraphicsAppSettings("Methane Hello Cube", Samples::g_default_app_options_color_only_and_anim);
                settings.graphics_app.SetScreenPassAccess(RenderPass::Access::None);
                return settings;
            }(),
            "Tutorial demonstrating colored rotating cube rendering with Methane Kit.")
        , m_proj_vertices(m_cube_mesh.GetVertices())
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

    ~HelloCubeApp() override
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
                        Program::ArgumentAccessors{ },
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

        // Create per-frame command lists
        for(HelloCubeFrame& frame : GetFrames())
        {
            // Create vertex buffers for each frame
            Ptr<Buffer> vertex_buffer_ptr = Buffer::CreateVertexBuffer(GetRenderContext(), m_cube_mesh.GetVertexDataSize(), m_cube_mesh.GetVertexSize(), true);
            vertex_buffer_ptr->SetName(IndexedName("Cube Vertex Buffer", frame.index));
            frame.vertex_buffer_set_ptr = BufferSet::CreateVertexBuffers({ *vertex_buffer_ptr });

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

        // Update vertex buffer with camera Model-View-Projection matrix applied on CPU
        const hlslpp::float4x4 mvp_matrix = hlslpp::mul(m_model_matrix, m_camera.GetViewProjMatrix());
        for(size_t vertex_index = 0; vertex_index < m_proj_vertices.size(); ++vertex_index)
        {
            const hlslpp::float4 orig_position_vec(m_cube_mesh.GetVertices()[vertex_index].position.AsHlsl(), 1.F);
            const hlslpp::float4 proj_position_vec = hlslpp::mul(orig_position_vec, mvp_matrix);
            m_proj_vertices[vertex_index].position = Mesh::Position(proj_position_vec.xyz / proj_position_vec.w);
        }

        return true;
    }

    bool Render() override
    {
        if (!GraphicsApp::Render())
            return false;

        const HelloCubeFrame& frame = GetCurrentFrame();

        // Update vertex buffer with vertices in camera's projection view
        (*frame.vertex_buffer_set_ptr)[0].SetData(
            { { reinterpret_cast<Data::ConstRawPtr>(m_proj_vertices.data()), m_cube_mesh.GetVertexDataSize() } }, // NOSONAR
            &GetRenderContext().GetRenderCommandKit().GetQueue()
        );

        // Issue commands for cube rendering
        META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Cube Rendering");
        frame.render_cmd_list_ptr->ResetWithState(*m_render_state_ptr, s_debug_group.get());
        frame.render_cmd_list_ptr->SetViewState(GetViewState());
        frame.render_cmd_list_ptr->SetVertexBuffers(*frame.vertex_buffer_set_ptr);
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
        m_index_buffer_ptr.reset();
        m_render_state_ptr.reset();

        GraphicsApp::OnContextReleased(context);
    }
};

int main(int argc, const char* argv[])
{
    return HelloCubeApp().Run({ argc, argv });
}
