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

#include "IApp.h"
#include "AppBase.h"
#include "AppController.h"

#include <Methane/Graphics/App.hpp>
#include <Methane/UserInterface/Context.h>
#include <Methane/Instrumentation.h>

namespace Methane::UserInterface
{

namespace gfx = Methane::Graphics;
namespace rhi = Methane::Graphics::Rhi;

template<typename FrameT>
class App
    : public Graphics::App<FrameT, UserInterface::IApp>
    , public AppBase
{
public:
    using GraphicsApp = Graphics::App<FrameT, UserInterface::IApp>;

    explicit App(const Graphics::CombinedAppSettings& graphics_app_settings,
                 const IApp::Settings& ui_app_settings = { },
                 const std::string& help_description = "Methane Graphics Application")
        : GraphicsApp(graphics_app_settings)
        , AppBase(ui_app_settings)
    {
        META_FUNCTION_TASK();
        CLI::App::add_option("-i,--hud", AppBase::GetAppSettings().heads_up_display_mode, "HUD display mode (0 - hidden, 1 - in window title, 2 - in UI)");
        Platform::App::AddInputControllers({ std::make_shared<AppController>(*this, help_description) });
        GraphicsApp::SetShowHudInWindowTitle(AppBase::GetAppSettings().heads_up_display_mode == HeadsUpDisplayMode::WindowTitle);
    }

    void Init() override
    {
        META_FUNCTION_TASK();
        // Update parameters since they could change after parsing command line arguments
        UpdateParametersText();
        GraphicsApp::Init();
        AppBase::InitUI(*this,
                        GraphicsApp::GetRenderContext().GetRenderCommandKit().GetQueue().GetInterface(),
                        GraphicsApp::GetScreenRenderPattern().GetInterface(),
                        GraphicsApp::GetFrameSize());
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

    const IApp::Settings& GetUserInterfaceAppSettings() const noexcept final { return AppBase::GetAppSettings(); }

    bool SetHeadsUpDisplayMode(UserInterface::HeadsUpDisplayMode heads_up_display_mode) final
    {
        META_FUNCTION_TASK();
        if (AppBase::GetAppSettings().heads_up_display_mode == heads_up_display_mode)
            return false;

        GraphicsApp::SetShowHudInWindowTitle(heads_up_display_mode == UserInterface::HeadsUpDisplayMode::WindowTitle);
        GraphicsApp::WaitForRenderComplete();

        return AppBase::SetHeadsUpDisplayUIMode(heads_up_display_mode);
    }

    bool SetAnimationsEnabled(bool animations_enabled) final
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
    void ShowControlsHelp() final
    {
        META_FUNCTION_TASK();
        if (!SetHelpText(Platform::AppBase::GetControlsHelp()))
            SetHelpText("");
    }

    void ShowCommandLineHelp() final
    {
        META_FUNCTION_TASK();
        if (!SetHelpText(Platform::AppBase::GetCommandLineHelp()))
            SetHelpText("");
    }

    // IContextCallback override
    void OnContextReleased(rhi::IContext& context) override
    {
        META_FUNCTION_TASK();
        AppBase::ReleaseUI();
        GraphicsApp::OnContextReleased(context);
    }
};

} // namespace Methane::UserInterface
