/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/UserInterface/App.hpp
Base template class of the Methane user interface application.

******************************************************************************/

#pragma once

#include "App.h"
#include "AppController.h"

#include <Methane/Graphics/App.hpp>
#include <Methane/UserInterface/Badge.h>
#include <Methane/UserInterface/HeadsUpDisplay.h>
#include <Methane/Instrumentation.h>

namespace Methane::UserInterface
{

namespace gfx = Methane::Graphics;

template<typename FrameT>
class App
    : public Graphics::App<FrameT, UserInterface::IApp>
{
public:
    using GraphicsApp = Graphics::App<FrameT, UserInterface::IApp>;

    App(const Graphics::AppSettings& graphics_app_settings,
        const UserInterface::IApp::Settings& ui_app_settings = UserInterface::IApp::Settings(),
        const std::string& help_description = "Methane Graphics Application")
        : GraphicsApp(graphics_app_settings)
        , m_app_settings(ui_app_settings)
    {
        META_FUNCTION_TASK();
        CLI::App::add_option("-i,--hud", m_app_settings.heads_up_display_mode, "HUD display mode (0 - hidden, 1 - in window title, 2 - in UI)", true);

        Platform::App::AddInputControllers({ std::make_shared<AppController>(*this, help_description) });
        GraphicsApp::SetShowHudInWindowTitle(m_app_settings.heads_up_display_mode == IApp::HeadsUpDisplayMode::WindowTitle);
    }

    void Init() override
    {
        META_FUNCTION_TASK();
        GraphicsApp::Init();
        
        // Create Methane logo badge
        if (m_app_settings.show_logo_badge)
        {
            Badge::Settings logo_badge_settings;
            logo_badge_settings.blend_color  = gfx::Color4f(1.f, 1.f, 1.f, 0.15f);
            m_sp_logo_badge = std::make_shared<Badge>(GraphicsApp::GetRenderContext(), std::move(logo_badge_settings));
        }

        // Create heads-up-display (HUD)
        if (m_app_settings.heads_up_display_mode == IApp::HeadsUpDisplayMode::UserInterface)
            m_sp_hud = std::make_shared<HeadsUpDisplay>(GraphicsApp::GetRenderContext(), m_hud_settings);
    }

    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override
    {
        META_FUNCTION_TASK();
        if (!GraphicsApp::Resize(frame_size, is_minimized))
            return false;
        
        if (m_sp_logo_badge)
            m_sp_logo_badge->FrameResize(frame_size);

        return true;
    }
    
    bool Update() override
    {
        META_FUNCTION_TASK();
        if (!GraphicsApp::Update())
            return false;

        // Update HUD user interface
        if (m_sp_hud && m_app_settings.heads_up_display_mode == IApp::HeadsUpDisplayMode::UserInterface)
            m_sp_hud->Update();

        return true;
    }
    
    void RenderOverlay(gfx::RenderCommandList& cmd_list)
    {
        META_FUNCTION_TASK();
        if (m_sp_hud && m_app_settings.heads_up_display_mode == IApp::HeadsUpDisplayMode::UserInterface)
            m_sp_hud->Draw(cmd_list);

        if (m_sp_logo_badge)
            m_sp_logo_badge->Draw(cmd_list);
    }

    // UserInterface::IApp interface

    const UserInterface::IApp::Settings& GetUserInterfaceAppSettings() const noexcept override { return m_app_settings; }

    bool SetHeadsUpDisplayMode(IApp::HeadsUpDisplayMode heads_up_display_mode) override
    {
        if (m_app_settings.heads_up_display_mode == heads_up_display_mode)
            return false;

        m_app_settings.heads_up_display_mode = heads_up_display_mode;

        GraphicsApp::SetShowHudInWindowTitle(m_app_settings.heads_up_display_mode == IApp::HeadsUpDisplayMode::WindowTitle);
        GraphicsApp::GetRenderContext().WaitForGpu(gfx::RenderContext::WaitFor::RenderComplete);

        if (m_app_settings.heads_up_display_mode == IApp::HeadsUpDisplayMode::UserInterface && GraphicsApp::IsRenderContextInitialized())
        {
            m_sp_hud = std::make_shared<HeadsUpDisplay>(GraphicsApp::GetRenderContext(), m_hud_settings);
        }
        else
        {
            m_sp_hud.reset();
        }
        return true;
    }

protected:
    // IContextCallback implementation
    void OnContextReleased(gfx::Context& context) override
    {
        META_FUNCTION_TASK();

        m_sp_logo_badge.reset();
        m_sp_hud.reset();

        GraphicsApp::OnContextReleased(context);
    }

    HeadsUpDisplay::Settings& GetHeadsUpDisplaySettings()        { return m_hud_settings; }
    HeadsUpDisplay*           GetHeadsUpDisplay() const noexcept { return m_sp_hud.get(); }

private:
    UserInterface::IApp::Settings m_app_settings;
    HeadsUpDisplay::Settings      m_hud_settings;
    Ptr<Badge>                    m_sp_logo_badge;
    Ptr<HeadsUpDisplay>           m_sp_hud;
};

} // namespace Methane::UserInterface
