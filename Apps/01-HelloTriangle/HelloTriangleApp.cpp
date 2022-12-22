/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: HelloTriangleApp.cpp
Tutorial demonstrating colored triangle rendering with Methane graphics API

******************************************************************************/

#include <Methane/Kit.h>
#include <Methane/Graphics/App.hpp>
#include <Methane/Tutorials/AppSettings.h>

using namespace Methane;
using namespace Methane::Graphics;

struct HelloTriangleFrame final
    : AppFrame
{
    Rhi::RenderCommandList render_cmd_list;
    Rhi::CommandListSet    execute_cmd_list_set;
    using AppFrame::AppFrame;
};

using GraphicsApp = Graphics::App<HelloTriangleFrame>;
class HelloTriangleApp final
    : public GraphicsApp // NOSONAR
{
private:
    Rhi::RenderState m_render_state;

public:
    HelloTriangleApp()
        : GraphicsApp(
            []() {
                Graphics::CombinedAppSettings settings = Tutorials::GetGraphicsTutorialAppSettings("Methane Hello Triangle", Tutorials::AppOptions::GetDefaultWithColorOnly());
                settings.graphics_app.SetScreenPassAccess({});
                return settings;
            }(),
            "Tutorial demonstrating colored triangle rendering with Methane Kit.")
    { }

    ~HelloTriangleApp() override
    {
        WaitForRenderComplete();
    }

    void Init() override
    {
        GraphicsApp::Init();

        m_render_state.Init(GetRenderContext(),
            Rhi::RenderState::Settings
            {
                Rhi::Program(GetRenderContext(),
                    Rhi::Program::Settings
                    {
                        Rhi::Program::ShaderSet
                        {
                            { Rhi::ShaderType::Vertex, { Data::ShaderProvider::Get(), { "HelloTriangle", "TriangleVS" } } },
                            { Rhi::ShaderType::Pixel,  { Data::ShaderProvider::Get(), { "HelloTriangle", "TrianglePS" } } },
                        },
                        Rhi::ProgramInputBufferLayouts{ },
                        Rhi::ProgramArgumentAccessors{ },
                        GetScreenRenderPattern().GetAttachmentFormats()
                    }
                ),
                GetScreenRenderPattern()
            }
        );
        m_render_state.SetName("Triangle Render State");

        for (HelloTriangleFrame& frame : GetFrames())
        {
            frame.render_cmd_list.Init(GetRenderContext().GetRenderCommandKit().GetQueue(), frame.screen_pass);
            frame.render_cmd_list.SetName(IndexedName("Render Triangle", frame.index));
            frame.execute_cmd_list_set.Init({ frame.render_cmd_list.GetInterface() }, frame.index);
        }

        GraphicsApp::CompleteInitialization();
    }

    bool Render() override
    {
        if (!GraphicsApp::Render())
            return false;

        const HelloTriangleFrame& frame = GetCurrentFrame();
        frame.render_cmd_list.ResetWithState(m_render_state);
        frame.render_cmd_list.SetViewState(GetViewState());
        frame.render_cmd_list.Draw(Rhi::RenderPrimitive::Triangle, 3);
        frame.render_cmd_list.Commit();

        GetRenderContext().GetRenderCommandKit().GetQueue().Execute(frame.execute_cmd_list_set);
        GetRenderContext().Present();

        return true;
    }

    void OnContextReleased(Rhi::IContext& context) override
    {
        m_render_state.Release();

        GraphicsApp::OnContextReleased(context);
    }
};

int main(int argc, const char* argv[])
{
    return HelloTriangleApp().Run({ argc, argv });
}
