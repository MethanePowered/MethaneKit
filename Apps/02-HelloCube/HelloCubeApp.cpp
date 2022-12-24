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
#include <Methane/Tutorials/AppSettings.h>
#include <Methane/Data/TimeAnimation.h>

#ifdef UNIFORMS_BUFFER_ENABLED
namespace hlslpp // NOSONAR
{
#pragma pack(push, 16)
#include "Shaders/HelloCubeUniforms.h" // NOSONAR
#pragma pack(pop)
}

static const std::string g_app_name = "Methane Hello Cube Uniforms";
#else
static const std::string g_app_name = "Methane Hello Cube Simple";
#endif

using namespace Methane;
using namespace Methane::Graphics;
using namespace Methane::Graphics::Rhi;

struct HelloCubeFrame final : AppFrame
{
#ifdef UNIFORMS_BUFFER_ENABLED
    Buffer            uniforms_buffer;
    ProgramBindings   program_bindings;
#else
    BufferSet         vertex_buffer_set;
#endif
    RenderCommandList render_cmd_list;
    CommandListSet    execute_cmd_list_set;

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
    Camera                     m_camera;

#ifdef UNIFORMS_BUFFER_ENABLED
    hlslpp::Uniforms              m_shader_uniforms { };
    const IResource::SubResources m_shader_uniforms_subresources{
        { reinterpret_cast<Data::ConstRawPtr>(&m_shader_uniforms), sizeof(hlslpp::Uniforms) } // NOSONAR
    };
    BufferSet m_vertex_buffer_set;
#else
    std::vector<CubeVertex> m_proj_vertices;
#endif

    CommandQueue m_render_cmd_queue;
    RenderState  m_render_state;
    Buffer       m_index_buffer;

public:
    HelloCubeApp()
        : GraphicsApp(
            []() {
                Graphics::CombinedAppSettings settings = Tutorials::GetGraphicsTutorialAppSettings(g_app_name, Tutorials::AppOptions::GetDefaultWithColorOnlyAndAnim());
#ifdef UNIFORMS_BUFFER_ENABLED
                settings.graphics_app.SetScreenPassAccess(Rhi::RenderPassAccessMask(RenderPassAccess::ShaderResources));
#else
                settings.graphics_app.SetScreenPassAccess({});
#endif
                return settings;
            }(),
            "Tutorial demonstrating colored rotating cube rendering with Methane Kit.")
#ifndef UNIFORMS_BUFFER_ENABLED
        , m_proj_vertices(m_cube_mesh.GetVertices())
#endif
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

#ifdef UNIFORMS_BUFFER_ENABLED
        const IShader::MacroDefinitions vertex_shader_definitions{ { "UNIFORMS_BUFFER_ENABLED", "" } };
#else
        const IShader::MacroDefinitions vertex_shader_definitions;
#endif

        // Create render state with program
        m_render_state.Init(GetRenderContext(),
            RenderState::Settings
            {
                Program(GetRenderContext(),
                    Program::Settings
                    {
                        Program::ShaderSet
                        {
                            { ShaderType::Vertex, { Data::ShaderProvider::Get(), { "HelloCube", "CubeVS" }, vertex_shader_definitions } },
                            { ShaderType::Pixel,  { Data::ShaderProvider::Get(), { "HelloCube", "CubePS" } } },
                        },
                        ProgramInputBufferLayouts
                        {
                            Program::InputBufferLayout
                            {
                                Program::InputBufferLayout::ArgumentSemantics { "POSITION" , "COLOR" }
                            }
                        },
#ifdef UNIFORMS_BUFFER_ENABLED
                        Rhi::ProgramArgumentAccessors
                        {
                            { { ShaderType::Vertex, "g_uniforms" }, Rhi::ProgramArgumentAccessType::FrameConstant }
                        },
#else
                        Rhi::ProgramArgumentAccessors{ },
#endif
                        GetScreenRenderPattern().GetAttachmentFormats()
                    }
                ),
                GetScreenRenderPattern()
            }
        );
        m_render_state.GetSettings().program_ptr->SetName("Colored Cube Shading");
        m_render_state.SetName("Colored Cube Pipeline State");

        // Create index buffer for cube mesh
        m_index_buffer.InitIndexBuffer(GetRenderContext().GetInterface(), m_cube_mesh.GetIndexDataSize(), GetIndexFormat(m_cube_mesh.GetIndex(0)));
        m_index_buffer.SetName("Cube Index Buffer");
        m_index_buffer.SetData(
            { { reinterpret_cast<Data::ConstRawPtr>(m_cube_mesh.GetIndices().data()), m_cube_mesh.GetIndexDataSize() } }, // NOSONAR
            GetRenderContext().GetRenderCommandKit().GetQueue()
        );

#ifdef UNIFORMS_BUFFER_ENABLED
        // Create constant vertex buffer
        Buffer vertex_buffer;
        vertex_buffer.InitVertexBuffer(GetRenderContext().GetInterface(), m_cube_mesh.GetVertexDataSize(), m_cube_mesh.GetVertexSize());
        vertex_buffer.SetName("Cube Vertex Buffer");
        vertex_buffer.SetData(
            { { reinterpret_cast<Data::ConstRawPtr>(m_cube_mesh.GetVertices().data()), m_cube_mesh.GetVertexDataSize() } }, // NOSONAR
            GetRenderContext().GetRenderCommandKit().GetQueue()
        );
        m_vertex_buffer_set.Init(BufferType::Vertex, { vertex_buffer });

        const auto uniforms_data_size = static_cast<Data::Size>(sizeof(m_shader_uniforms));
#endif

        // Create per-frame command lists
        for(HelloCubeFrame& frame : GetFrames())
        {
#ifdef UNIFORMS_BUFFER_ENABLED
            // Create uniforms buffer with volatile parameters for frame rendering
            frame.uniforms_buffer.InitConstantBuffer(GetRenderContext().GetInterface(), uniforms_data_size, false, true);
            frame.uniforms_buffer.SetName(IndexedName("Uniforms Buffer", frame.index));

            // Configure program resource bindings
            frame.program_bindings.Init(*m_render_state.GetSettings().program_ptr, {
                { { ShaderType::Vertex, "g_uniforms"  }, { { frame.uniforms_buffer.GetInterface() } } }
            }, frame.index);
            frame.program_bindings.SetName(IndexedName("Cube Bindings {}", frame.index));
#else
            // Create vertex buffers for each frame
            Buffer vertex_buffer;
            vertex_buffer.InitVertexBuffer(GetRenderContext().GetInterface(), m_cube_mesh.GetVertexDataSize(), m_cube_mesh.GetVertexSize(), true);
            vertex_buffer.SetName(IndexedName("Cube Vertex Buffer", frame.index));
            frame.vertex_buffer_set.Init(BufferType::Vertex, { vertex_buffer });
#endif

            // Create command list for rendering
            frame.render_cmd_list.Init(GetRenderContext().GetRenderCommandKit().GetQueue(), frame.screen_pass);
            frame.render_cmd_list.SetName(IndexedName("Cube Rendering", frame.index));
            frame.execute_cmd_list_set.Init({ frame.render_cmd_list.GetInterface() }, frame.index);
        }

        m_render_cmd_queue = GetRenderContext().GetRenderCommandKit().GetQueue();

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

        const hlslpp::float4x4 mvp_matrix = hlslpp::mul(m_model_matrix, m_camera.GetViewProjMatrix());

#ifdef UNIFORMS_BUFFER_ENABLED
        // Save transposed camera Model-View-Projection matrix in shader uniforms to be uploaded in uniforms buffer on GPU
        m_shader_uniforms.mvp_matrix = hlslpp::transpose(mvp_matrix);
#else
        // Update vertex buffer with camera Model-View-Projection matrix applied on CPU
        for(size_t vertex_index = 0; vertex_index < m_proj_vertices.size(); ++vertex_index)
        {
            const hlslpp::float4 orig_position_vec(m_cube_mesh.GetVertices()[vertex_index].position.AsHlsl(), 1.F);
            const hlslpp::float4 proj_position_vec = hlslpp::mul(orig_position_vec, mvp_matrix);
            m_proj_vertices[vertex_index].position = Mesh::Position(proj_position_vec.xyz / proj_position_vec.w);
        }
#endif

        return true;
    }

    bool Render() override
    {
        if (!GraphicsApp::Render())
            return false;

        const HelloCubeFrame& frame = GetCurrentFrame();

#ifdef UNIFORMS_BUFFER_ENABLED
        // Update uniforms buffer on GPU and apply model-view-projection tranformation in vertex shader on GPU
        frame.uniforms_buffer.SetData(m_shader_uniforms_subresources, m_render_cmd_queue);
#else
        // Update vertex buffer with vertices in camera's projection view
        (frame.vertex_buffer_set)[0].SetData(
            { { reinterpret_cast<Data::ConstRawPtr>(m_proj_vertices.data()), m_cube_mesh.GetVertexDataSize() } }, // NOSONAR
            m_render_cmd_queue
        );
#endif

        // Issue commands for cube rendering
        static META_DEBUG_GROUP_VAR(s_debug_group, "Cube Rendering");
        frame.render_cmd_list.ResetWithState(m_render_state, &s_debug_group);
        frame.render_cmd_list.SetViewState(GetViewState());
#ifdef UNIFORMS_BUFFER_ENABLED
        frame.render_cmd_list.SetProgramBindings(frame.program_bindings);
        frame.render_cmd_list.SetVertexBuffers(m_vertex_buffer_set);
#else
        frame.render_cmd_list.SetVertexBuffers(frame.vertex_buffer_set);
#endif
        frame.render_cmd_list.SetIndexBuffer(m_index_buffer);
        frame.render_cmd_list.DrawIndexed(RenderPrimitive::Triangle);
        frame.render_cmd_list.Commit();

        // Execute command list on render queue and present frame to screen
        m_render_cmd_queue.Execute(frame.execute_cmd_list_set);
        GetRenderContext().Present();

        return true;
    }

    void OnContextReleased(Rhi::IContext& context) override
    {
#ifdef UNIFORMS_BUFFER_EANBLED
        m_vertex_buffer_set.Release();
#endif
        m_index_buffer.Release();
        m_render_state.Release();

        GraphicsApp::OnContextReleased(context);
    }
};

int main(int argc, const char* argv[])
{
    return HelloCubeApp().Run({ argc, argv });
}
