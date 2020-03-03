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

#include <Methane/Graphics/Kit.h>

using namespace Methane;
using namespace Methane::Graphics;

struct HelloTriangleFrame final : AppFrame
{
    Ptr<RenderCommandList> sp_cmd_list;
    using AppFrame::AppFrame;
};

using GraphicsApp = App<HelloTriangleFrame>;
class HelloTriangleApp final : public GraphicsApp
{
private:
    Ptr<RenderState> m_sp_state;
    Ptr<Buffer>      m_sp_vertex_buffer;

public:
    HelloTriangleApp() : GraphicsApp(
        {                                        // Application settings:
            {                                    // platform_app:
                "Methane Hello Triangle",        // - name
                0.8, 0.8,                        // - width, height
            },                                   //
            {                                    // graphics_app:
                RenderPass::Access::None,        // - screen_pass_access
                false,                           // - animations_enabled
                true,                            // - show_hud_in_window_title
                false,                           // - show_logo_badge
            },                                   //
            {                                    // render_context:
                FrameSize(),                     // - frame_size placeholder: actual size is set in InitContext
                PixelFormat::BGRA8Unorm,         // - color_format
                PixelFormat::Unknown,            // - depth_stencil_format
                Color4f(0.0f, 0.2f, 0.4f, 1.0f), // - clear_color
            }
        })
    { }

    ~HelloTriangleApp() override
    {
        m_sp_context->WaitForGpu(Context::WaitFor::RenderComplete);
    }

    void Init() override
    {
        GraphicsApp::Init();

        struct Vertex { Vector3f position; Vector3f color; };
        const std::array<Vertex, 3> triangle_vertices = { {
            { { 0.0f,   0.5f,  0.0f }, { 1.0f, 0.0f, 0.0f } },
            { { 0.5f,  -0.5f,  0.0f }, { 0.0f, 1.0f, 0.0f } },
            { { -0.5f, -0.5f,  0.0f }, { 0.0f, 0.0f, 1.0f } },
        } };

        const Data::Size vertex_buffer_size = static_cast<Data::Size>(sizeof(triangle_vertices));
        m_sp_vertex_buffer = Buffer::CreateVertexBuffer(*m_sp_context, vertex_buffer_size, static_cast<Data::Size>(sizeof(Vertex)));
        m_sp_vertex_buffer->SetData(
            Resource::SubResources
            {
                Resource::SubResource { reinterpret_cast<Data::ConstRawPtr>(triangle_vertices.data()), vertex_buffer_size }
            }
        );

        m_sp_state = RenderState::Create(*m_sp_context,
            RenderState::Settings
            {
                Program::Create(*m_sp_context,
                    Program::Settings
                    {
                        Program::Shaders
                        {
                            Shader::CreateVertex(*m_sp_context, { Data::ShaderProvider::Get(), { "Triangle", "TriangleVS" } }),
                            Shader::CreatePixel(*m_sp_context,  { Data::ShaderProvider::Get(), { "Triangle", "TrianglePS" } }),
                        },
                        Program::InputBufferLayouts
                        {
                            Program::InputBufferLayout
                            {
                                Program::InputBufferLayout::ArgumentSemantics { "POSITION", "COLOR" },
                            }
                        },
                        Program::ArgumentDescriptions { },
                        PixelFormats { m_sp_context->GetSettings().color_format }
                    }
                ),
                Viewports    { GetFrameViewport(m_sp_context->GetSettings().frame_size)    },
                ScissorRects { GetFrameScissorRect(m_sp_context->GetSettings().frame_size) },
            }
        );

        for (HelloTriangleFrame& frame : m_frames)
        {
            frame.sp_cmd_list = RenderCommandList::Create(m_sp_context->GetRenderCommandQueue(), *frame.sp_screen_pass);
        }

        m_sp_context->CompleteInitialization();
    }

    bool Resize(const FrameSize& frame_size, bool is_minimized) override
    {
        if (!GraphicsApp::Resize(frame_size, is_minimized))
            return false;

        m_sp_state->SetViewports(    { GetFrameViewport(frame_size)    } );
        m_sp_state->SetScissorRects( { GetFrameScissorRect(frame_size) } );
        return true;
    }

    bool Render() override
    {
        if (!m_sp_context->ReadyToRender() || !GraphicsApp::Render())
            return false;

        m_sp_context->WaitForGpu(Context::WaitFor::FramePresented);
        HelloTriangleFrame& frame = GetCurrentFrame();

        frame.sp_cmd_list->Reset(m_sp_state);
        frame.sp_cmd_list->SetVertexBuffers({ *m_sp_vertex_buffer });
        frame.sp_cmd_list->Draw(RenderCommandList::Primitive::Triangle, 3);
        frame.sp_cmd_list->Commit();

        m_sp_context->GetRenderCommandQueue().Execute({ *frame.sp_cmd_list });
        m_sp_context->Present();

        return true;
    }

    void OnContextReleased() override
    {
        m_sp_vertex_buffer.reset();
        m_sp_state.reset();

        GraphicsApp::OnContextReleased();
    }
};

int main(int argc, const char* argv[])
{
    return HelloTriangleApp().Run({ argc, argv });
}
