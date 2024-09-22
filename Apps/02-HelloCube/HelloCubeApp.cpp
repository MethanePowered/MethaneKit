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
#include <Methane/Graphics/TypeConverters.hpp>
#include <Methane/Tutorials/AppSettings.h>
#include <Methane/Data/TimeAnimation.h>

#ifdef UNIFORMS_ENABLED
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

struct HelloCubeFrame final : AppFrame
{
#ifdef UNIFORMS_ENABLED
    Rhi::ProgramBindings   program_bindings;
#else
    Rhi::BufferSet         vertex_buffer_set;
#endif
    Rhi::RenderCommandList render_cmd_list;
    Rhi::CommandListSet    execute_cmd_list_set;

    using AppFrame::AppFrame;
};

using GraphicsApp = App<HelloCubeFrame>;
class HelloCubeApp final // NOSONAR - destructor required
    : public GraphicsApp
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

#ifdef UNIFORMS_ENABLED
    Rhi::BufferSet   m_vertex_buffer_set;
#else
    std::vector<CubeVertex> m_proj_vertices;
#endif

    Rhi::CommandQueue m_render_cmd_queue;
    Rhi::RenderState  m_render_state;
    Rhi::Buffer       m_index_buffer;

public:
    HelloCubeApp()
        : GraphicsApp(
            []() {
                Graphics::CombinedAppSettings settings = Tutorials::GetGraphicsTutorialAppSettings(g_app_name, Tutorials::AppOptions::GetDefaultWithColorOnlyAndAnim());
#ifdef UNIFORMS_ENABLED
                settings.graphics_app.SetScreenPassAccess(Rhi::RenderPassAccessMask(Rhi::RenderPassAccess::ShaderResources));
#else
                settings.graphics_app.SetScreenPassAccess({});
#endif
                return settings;
            }(),
            "Tutorial demonstrating colored rotating cube rendering with Methane Kit.")
#ifndef UNIFORMS_ENABLED
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

#ifdef UNIFORMS_ENABLED
        const Rhi::Shader::MacroDefinitions vertex_shader_definitions{ { "UNIFORMS_ENABLED", "" } };
#else
        const Rhi::Shader::MacroDefinitions vertex_shader_definitions;
#endif

        // Create render state with program
        m_render_state = GetRenderContext().CreateRenderState(
            Rhi::RenderState::Settings
            {
                GetRenderContext().CreateProgram(
                    Rhi::Program::Settings
                    {
                        Rhi::Program::ShaderSet
                        {
                            { Rhi::ShaderType::Vertex, { Data::ShaderProvider::Get(), { "HelloCube", "CubeVS" }, vertex_shader_definitions } },
                            { Rhi::ShaderType::Pixel,  { Data::ShaderProvider::Get(), { "HelloCube", "CubePS" } } },
                        },
                        Rhi::ProgramInputBufferLayouts
                        {
                            Rhi::ProgramInputBufferLayout
                            {
                                Rhi::ProgramInputBufferLayout::ArgumentSemantics{ "POSITION" , "COLOR" }
                            }
                        },
                        Rhi::ProgramArgumentAccessors
                        {
#ifdef UNIFORMS_ENABLED
                            META_PROGRAM_ARG_ROOT_FRAME_CONSTANT(Rhi::ShaderType::Vertex, "g_uniforms")
#endif
                        },
                        GetScreenRenderPattern().GetAttachmentFormats()
                    }
                ),
                GetScreenRenderPattern()
            }
        );
        m_render_state.GetSettings().program_ptr->SetName("Colored Cube Shading");
        m_render_state.SetName("Colored Cube Pipeline State");

        m_render_cmd_queue = GetRenderContext().GetRenderCommandKit().GetQueue();

        // Create index buffer for cube mesh
        m_index_buffer = GetRenderContext().CreateBuffer(Rhi::BufferSettings::ForIndexBuffer(m_cube_mesh.GetIndexDataSize(), GetIndexFormat(m_cube_mesh.GetIndex(0))));
        m_index_buffer.SetName("Cube Index Buffer");
        m_index_buffer.SetData(m_render_cmd_queue, {
            reinterpret_cast<Data::ConstRawPtr>(m_cube_mesh.GetIndices().data()), // NOSONAR
            m_cube_mesh.GetIndexDataSize()
        });

#ifdef UNIFORMS_ENABLED
        // Create constant vertex buffer
        Rhi::Buffer vertex_buffer = GetRenderContext().CreateBuffer(Rhi::BufferSettings::ForVertexBuffer(m_cube_mesh.GetVertexDataSize(), m_cube_mesh.GetVertexSize()));
        vertex_buffer.SetName("Cube Vertex Buffer");
        vertex_buffer.SetData(m_render_cmd_queue, {
            reinterpret_cast<Data::ConstRawPtr>(m_cube_mesh.GetVertices().data()), // NOSONAR
            m_cube_mesh.GetVertexDataSize()
        });
        m_vertex_buffer_set = Rhi::BufferSet(Rhi::BufferType::Vertex, { vertex_buffer });
#endif

        // Create per-frame command lists
        for(HelloCubeFrame& frame : GetFrames())
        {
#ifdef UNIFORMS_ENABLED
            // Configure program resource bindings
            frame.program_bindings = m_render_state.GetProgram().CreateBindings({ }, frame.index);
            frame.program_bindings.SetName(fmt::format("Cube Bindings {}", frame.index));
#else
            // Create vertex buffers for each frame
            Rhi::Buffer vertex_buffer = GetRenderContext().CreateBuffer(Rhi::BufferSettings::ForVertexBuffer(m_cube_mesh.GetVertexDataSize(), m_cube_mesh.GetVertexSize(), true));
            vertex_buffer.SetName(fmt::format("Cube Vertex Buffer {}", frame.index));
            frame.vertex_buffer_set = Rhi::BufferSet(Rhi::BufferType::Vertex, { vertex_buffer });
#endif

            // Create command list for rendering
            frame.render_cmd_list = m_render_cmd_queue.CreateRenderCommandList(frame.screen_pass);
            frame.render_cmd_list.SetName(fmt::format("Cube Rendering {}", frame.index));
            frame.execute_cmd_list_set = Rhi::CommandListSet({ frame.render_cmd_list.GetInterface() }, frame.index);
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

        const HelloCubeFrame& frame = GetCurrentFrame();
        const hlslpp::float4x4 mvp_matrix = hlslpp::mul(m_model_matrix, m_camera.GetViewProjMatrix());

#ifdef UNIFORMS_ENABLED
        // Save transposed camera Model-View-Projection matrix in shader uniforms
        // Before frame rendering set uniforms to root constant buffer to be uploaded to GPU
        hlslpp::Uniforms shader_uniforms{ hlslpp::transpose(mvp_matrix) };

        // Update root constant in program bindings to apply model-view-projection transformation in vertex shader on GPU
        frame.program_bindings.Get({ Rhi::ShaderType::Vertex, "g_uniforms" })
                              .SetRootConstant(Rhi::RootConstant(shader_uniforms));
#else
        // Update vertex buffer with camera Model-View-Projection matrix applied on CPU
        for(size_t vertex_index = 0; vertex_index < m_proj_vertices.size(); ++vertex_index)
        {
            const hlslpp::float4 orig_position_vec(m_cube_mesh.GetVertices()[vertex_index].position.AsHlsl(), 1.F);
            const hlslpp::float4 proj_position_vec = hlslpp::mul(orig_position_vec, mvp_matrix);
            m_proj_vertices[vertex_index].position = Mesh::Position(proj_position_vec.xyz / proj_position_vec.w);
        }

        // Update vertex buffer with vertices in camera's projection view
        frame.vertex_buffer_set[0].SetData(m_render_cmd_queue, {
            reinterpret_cast<Data::ConstRawPtr>(m_proj_vertices.data()), // NOSONAR
            m_cube_mesh.GetVertexDataSize()
        });
#endif

        return true;
    }

    bool Render() override
    {
        if (!GraphicsApp::Render())
            return false;

        const HelloCubeFrame& frame = GetCurrentFrame();

        // Issue commands for cube rendering
        META_DEBUG_GROUP_VAR(s_debug_group, "Cube Rendering");
        frame.render_cmd_list.ResetWithState(m_render_state, &s_debug_group);
        frame.render_cmd_list.SetViewState(GetViewState());
#ifdef UNIFORMS_ENABLED
        frame.render_cmd_list.SetProgramBindings(frame.program_bindings);
        frame.render_cmd_list.SetVertexBuffers(m_vertex_buffer_set);
#else
        frame.render_cmd_list.SetVertexBuffers(frame.vertex_buffer_set);
#endif
        frame.render_cmd_list.SetIndexBuffer(m_index_buffer);
        frame.render_cmd_list.DrawIndexed(Rhi::RenderPrimitive::Triangle);
        frame.render_cmd_list.Commit();

        // Execute command list on render queue and present frame to screen
        m_render_cmd_queue.Execute(frame.execute_cmd_list_set);
        GetRenderContext().Present();

        return true;
    }

    void OnContextReleased(Rhi::IContext& context) override
    {
#ifdef UNIFORMS_BUFFER_EANBLED
        m_vertex_buffer_set = {};
#endif
        m_index_buffer     = {};
        m_render_state     = {};
        m_render_cmd_queue = {};

        GraphicsApp::OnContextReleased(context);
    }
};

int main(int argc, const char* argv[])
{
    return HelloCubeApp().Run({ argc, argv });
}
