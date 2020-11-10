/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/UserInterface/App.hpp
Base template class of the Methane user interface application.

******************************************************************************/

#pragma once

#include "App.h"
#include "AppBase.h"
#include "AppController.h"

#include <Methane/Graphics/App.hpp>
#include <Methane/UserInterface/Context.h>
#include <Methane/Instrumentation.h>

namespace Methane::UserInterface
{

namespace gfx = Methane::Graphics;

template<typename FrameT>
class App
    : public Graphics::App<FrameT, IApp>
    , protected AppBase
{
public:
    using GraphicsApp = Graphics::App<FrameT, IApp>;

    explicit App(const Graphics::AppSettings& graphics_app_settings,
                 const IApp::Settings& ui_app_settings = { },
                 const std::string& help_description = "Methane Graphics Application")
        : GraphicsApp(graphics_app_settings)
        , AppBase(ui_app_settings)
    {
        META_FUNCTION_TASK();
        CLI::App::add_option("-i,--hud", AppBase::GetAppSettings().heads_up_display_mode, "HUD display mode (0 - hidden, 1 - in window title, 2 - in UI)", true);
        Platform::App::AddInputControllers({ std::make_shared<AppController>(*this, help_description) });
        GraphicsApp::SetShowHudInWindowTitle(AppBase::GetAppSettings().heads_up_display_mode == IApp::HeadsUpDisplayMode::WindowTitle);
    }

    void Init() override
    {
        META_FUNCTION_TASK();
        // Update parameters since they could change after parsing command line arguments
        UpdateParametersText();
        GraphicsApp::Init();
        AppBase::InitUI(GraphicsApp::GetRenderContext(), GraphicsApp::GetFrameSize());
    }

    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override
    {
        META_FUNCTION_TASK();
        if (!GraphicsApp::Resize(frame_size, is_minimized))
            return false;
        
        AppBase::ResizeUI(frame_size, is_minimized);
        return true;
    }
    
    bool Update() override
    {
        META_FUNCTION_TASK();
        if (!GraphicsApp::Update())
            return false;

        AppBase::UpdateUI();
        return true;
    }

    // UserInterface::IApp interface

    const IApp::Settings& GetUserInterfaceAppSettings() const noexcept override { return AppBase::GetAppSettings(); }

    bool SetHeadsUpDisplayMode(IApp::HeadsUpDisplayMode heads_up_display_mode) override
    {
        META_FUNCTION_TASK();
        if (AppBase::GetAppSettings().heads_up_display_mode == heads_up_display_mode)
            return false;

        GraphicsApp::SetShowHudInWindowTitle(heads_up_display_mode == IApp::HeadsUpDisplayMode::WindowTitle);
        GraphicsApp::GetRenderContext().WaitForGpu(gfx::RenderContext::WaitFor::RenderComplete);

        return AppBase::SetHeadsUpDisplayMode(heads_up_display_mode);
    }

    bool SetAnimationsEnabled(bool animations_enabled) override
    {
        META_FUNCTION_TASK();
        if (!GraphicsApp::SetAnimationsEnabled(animations_enabled))
            return false;

        UpdateParametersText();
        return true;
    }

    void ShowParameters() final
    {
        META_FUNCTION_TASK();
        if (IsParametersTextDisplayed())
            SetParametersText("");
        else
            SetParametersText(GetParametersString());
    }

    std::string GetParametersString() override { return ""; }

protected:
    void UpdateParametersText()
    {
        META_FUNCTION_TASK();
        if (IsParametersTextDisplayed())
            SetParametersText(GetParametersString());
    }

    // Platform::AppBase overrides
    void ShowControlsHelp() override
    {
        META_FUNCTION_TASK();
        if (!SetHelpText(Platform::AppBase::GetControlsHelp()))
            SetHelpText("");
    }

    void ShowCommandLineHelp() override
    {
        META_FUNCTION_TASK();
        if (!SetHelpText(Platform::AppBase::GetCommandLineHelp()))
            SetHelpText("");
    }

    // IContextCallback implementation
    void OnContextReleased(gfx::Context& context) override
    {
        META_FUNCTION_TASK();
        AppBase::ReleaseUI();
        GraphicsApp::OnContextReleased(context);
    }
};

} // namespace Methane::UserInterface
