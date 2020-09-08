/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: HelloTriangleAppSimple.cpp
Simplified tutorial demonstrating triangle rendering with Methane graphics API

******************************************************************************/

#include <Methane/Kit.h>

using namespace Methane;
using namespace Methane::Graphics;

struct HelloTriangleFrame final : AppFrame
{
    Ptr<RenderCommandList> render_cmd_list_ptr;
    Ptr<CommandListSet>    execute_cmd_list_set_ptr;
    using AppFrame::AppFrame;
};

using GraphicsApp = App<HelloTriangleFrame>;
class HelloTriangleApp final : public GraphicsApp
{
private:
    Ptr<RenderState> m_render_state_ptr;
    Ptr<BufferSet>   m_vertex_buffer_set_ptr;

public:
    HelloTriangleApp() : GraphicsApp(
        {                                               // Application settings:
            {                                           // platform_app:
                "Methane Hello Triangle",               // - name
                0.8, 0.8,                               // - width, height
            },                                          //
            {                                           // graphics_app:
                RenderPass::Access::None,               // - screen_pass_access
                false,                                  // - animations_enabled
            },                                          //
            {                                           // render_context:
                FrameSize(),                            // - frame_size placeholder: set in InitContext
                PixelFormat::BGRA8Unorm,                // - color_format
                PixelFormat::Unknown,                   // - depth_stencil_format
                Color4f(0.0f, 0.2f, 0.4f, 1.0f),        // - clear_color
            }
        })
    { }

    ~HelloTriangleApp() override
    {
        GetRenderContext().WaitForGpu(Context::WaitFor::RenderComplete);
    }

    void Init() override
    {
        GraphicsApp::Init();

        struct Vertex { Vector3f position; Vector3f color; };
        const std::array<Vertex, 3> triangle_vertices{ {
            { { 0.0f,   0.5f,  0.0f }, { 1.0f, 0.0f, 0.0f } },
            { { 0.5f,  -0.5f,  0.0f }, { 0.0f, 1.0f, 0.0f } },
            { { -0.5f, -0.5f,  0.0f }, { 0.0f, 0.0f, 1.0f } },
        } };

        const Data::Size vertex_buffer_size = static_cast<Data::Size>(sizeof(triangle_vertices));
        Ptr<Buffer> vertex_buffer_ptr = Buffer::CreateVertexBuffer(GetRenderContext(), vertex_buffer_size, static_cast<Data::Size>(sizeof(Vertex)));
        vertex_buffer_ptr->SetData(
            Resource::SubResources
            {
                Resource::SubResource { reinterpret_cast<Data::ConstRawPtr>(triangle_vertices.data()), vertex_buffer_size }
            }
        );
        m_vertex_buffer_set_ptr = BufferSet::CreateVertexBuffers({ *vertex_buffer_ptr });

        m_render_state_ptr = RenderState::Create(GetRenderContext(),
            RenderState::Settings
            {
                Program::Create(GetRenderContext(),
                    Program::Settings
                    {
                        Program::Shaders
                        {
                            Shader::CreateVertex(GetRenderContext(), { Data::ShaderProvider::Get(), { "Triangle", "TriangleVS" } }),
                            Shader::CreatePixel(GetRenderContext(),  { Data::ShaderProvider::Get(), { "Triangle", "TrianglePS" } }),
                        },
                        Program::InputBufferLayouts
                        {
                            Program::InputBufferLayout
                            {
                                Program::InputBufferLayout::ArgumentSemantics { "POSITION", "COLOR" },
                            }
                        },
                        Program::ArgumentDescriptions { },
                        PixelFormats { GetRenderContext().GetSettings().color_format }
                    }
                )
            }
        );

        for (HelloTriangleFrame& frame : GetFrames())
        {
            frame.render_cmd_list_ptr      = RenderCommandList::Create(GetRenderContext().GetRenderCommandQueue(), *frame.screen_pass_ptr);
            frame.execute_cmd_list_set_ptr = CommandListSet::Create({ *frame.render_cmd_list_ptr });
        }

        GraphicsApp::CompleteInitialization();
    }

    bool Render() override
    {
        if (!GraphicsApp::Render())
            return false;

        HelloTriangleFrame& frame = GetCurrentFrame();
        frame.render_cmd_list_ptr->Reset(m_render_state_ptr);
        frame.render_cmd_list_ptr->SetViewState(GetViewState());
        frame.render_cmd_list_ptr->SetVertexBuffers(*m_vertex_buffer_set_ptr);
        frame.render_cmd_list_ptr->Draw(RenderCommandList::Primitive::Triangle, 3);
        frame.render_cmd_list_ptr->Commit();

        GetRenderContext().GetRenderCommandQueue().Execute(*frame.execute_cmd_list_set_ptr);
        GetRenderContext().Present();

        return true;
    }

    void OnContextReleased(Context& context) override
    {
        m_vertex_buffer_set_ptr.reset();
        m_render_state_ptr.reset();

        GraphicsApp::OnContextReleased(context);
    }
};

int main(int argc, const char* argv[])
{
    return HelloTriangleApp().Run({ argc, argv });
}
