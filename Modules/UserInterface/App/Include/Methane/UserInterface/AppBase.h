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

FILE: Methane/UserInterface/AppBase.h
Base implementation of the Methane user interface application.

******************************************************************************/

#pragma once

#include "App.h"

#include <Methane/UserInterface/HeadsUpDisplay.h>

namespace Methane::UserInterface
{

namespace gfx = Methane::Graphics;

class Font;
class Badge;
class Text;

class AppBase
{
public:
    AppBase(const Ptr<gfx::RenderContext>& sp_render_context, const IApp::Settings& ui_app_settings);

    void Init();
    void Release();
    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized);
    bool Update();
    void RenderOverlay(gfx::RenderCommandList& cmd_list);

    bool SetHeadsUpDisplayMode(IApp::HeadsUpDisplayMode heads_up_display_mode);
    bool SetHelpText(const std::string& help_str);
    bool SetParametersText(const std::string& parameters_str);

    bool IsHelpTextDisplayed() const noexcept                               { return !!m_sp_help_text; }
    bool IsParametersTextDisplayed() const noexcept                         { return !!m_sp_parameters_text; }
    void GetParametersText(const std::string& parameters_str);
    Font& GetMainFont();

    const UserInterface::IApp::Settings& GetAppSettings() const noexcept    { return m_app_settings; }
    HeadsUpDisplay::Settings&            GetHeadsUpDisplaySettings()        { return m_hud_settings; }
    HeadsUpDisplay*                      GetHeadsUpDisplay() const noexcept { return m_sp_hud.get(); }

protected:
    UserInterface::IApp::Settings& GetAppSettings() noexcept { return m_app_settings; }

private:
    bool UpdateText(Ptr<Text>& sp_text, const std::string& help_str);
    void UpdateHelpTextPosition();
    void UpdateParametersTextPosition();

    const Ptr<gfx::RenderContext>& m_sp_render_context;
    IApp::Settings                 m_app_settings;
    HeadsUpDisplay::Settings       m_hud_settings;
    Graphics::FrameSize            m_frame_size;
    Graphics::FrameSize            m_text_margins;
    Ptr<Badge>                     m_sp_logo_badge;
    Ptr<HeadsUpDisplay>            m_sp_hud;
    Ptr<Font>                      m_sp_main_font;
    Ptr<Text>                      m_sp_help_text;
    Ptr<Text>                      m_sp_parameters_text;
};

} // namespace Methane::UserInterface
