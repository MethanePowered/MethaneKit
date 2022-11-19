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
using namespace Methane::Graphics::Rhi;

struct HelloTriangleFrame final : AppFrame
{
    Ptr<IRenderCommandList> render_cmd_list_ptr;
    Ptr<ICommandListSet>    execute_cmd_list_set_ptr;
    using AppFrame::AppFrame;
};

using GraphicsApp = Graphics::App<HelloTriangleFrame>;
class HelloTriangleApp final : public GraphicsApp // NOSONAR
{
private:
    Ptr<IRenderState> m_render_state_ptr;

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

        m_render_state_ptr = IRenderState::Create(GetRenderContext(),
            IRenderState::Settings
            {
                IProgram::Create(GetRenderContext(),
                    IProgram::Settings
                    {
                        IProgram::Shaders
                        {
                            IShader::CreateVertex(GetRenderContext(), { Data::ShaderProvider::Get(), { "HelloTriangle", "TriangleVS" } }),
                            IShader::CreatePixel(GetRenderContext(),  { Data::ShaderProvider::Get(), { "HelloTriangle", "TrianglePS" } }),
                        },
                        ProgramInputBufferLayouts{ },
                        Rhi::ProgramArgumentAccessors{ },
                        GetScreenRenderPattern().GetAttachmentFormats()
                    }
                ),
                GetScreenRenderPatternPtr()
            }
        );
        m_render_state_ptr->SetName("Triangle Render State");

        for (HelloTriangleFrame& frame : GetFrames())
        {
            frame.render_cmd_list_ptr = IRenderCommandList::Create(GetRenderContext().GetRenderCommandKit().GetQueue(), *frame.screen_pass_ptr);
            frame.render_cmd_list_ptr->SetName(IndexedName("Render Triangle", frame.index));
            frame.execute_cmd_list_set_ptr = ICommandListSet::Create({ *frame.render_cmd_list_ptr }, frame.index);
        }

        GraphicsApp::CompleteInitialization();
    }

    bool Render() override
    {
        if (!GraphicsApp::Render())
            return false;

        const HelloTriangleFrame& frame = GetCurrentFrame();
        frame.render_cmd_list_ptr->ResetWithState(*m_render_state_ptr);
        frame.render_cmd_list_ptr->SetViewState(GetViewState());
        frame.render_cmd_list_ptr->Draw(RenderPrimitive::Triangle, 3);
        frame.render_cmd_list_ptr->Commit();

        GetRenderContext().GetRenderCommandKit().GetQueue().Execute(*frame.execute_cmd_list_set_ptr);
        GetRenderContext().Present();

        return true;
    }

    void OnContextReleased(Rhi::IContext& context) override
    {
        m_render_state_ptr.reset();

        GraphicsApp::OnContextReleased(context);
    }
};

int main(int argc, const char* argv[])
{
    return HelloTriangleApp().Run({ argc, argv });
}
