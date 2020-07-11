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

FILE: Methane/UserInterface/AppBase.cpp
Base implementation of the Methane user interface application.

******************************************************************************/

#include <Methane/UserInterface/AppBase.h>

#include <Methane/UserInterface/Text.h>
#include <Methane/UserInterface/Badge.h>
#include <Methane/UserInterface/HeadsUpDisplay.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

namespace Methane::UserInterface
{

AppBase::AppBase(const Ptr<gfx::RenderContext>& sp_render_context, const IApp::Settings& ui_app_settings)
    : m_sp_render_context(sp_render_context)
    , m_app_settings(ui_app_settings)
{
    META_FUNCTION_TASK();
    m_hud_settings.text_color = m_app_settings.text_color;
}

void AppBase::Init()
{
    META_FUNCTION_TASK();
    if (!m_sp_render_context)
        throw std::logic_error("User interface app can not be initialized without render context.");

    m_text_margins = m_app_settings.text_margins_in_dots * m_sp_render_context->GetContentScalingFactor();

    // Create Methane logo badge
    if (m_app_settings.show_logo_badge)
    {
        Badge::Settings logo_badge_settings;
        logo_badge_settings.blend_color  = gfx::Color4f(1.f, 1.f, 1.f, 0.15f);
                            m_sp_logo_badge = std::make_shared<Badge>(*m_sp_render_context, std::move(logo_badge_settings));
    }

    // Create heads-up-display (HUD)
    m_hud_settings.position = { m_text_margins.width, m_text_margins.height };
    if (m_app_settings.heads_up_display_mode == IApp::HeadsUpDisplayMode::UserInterface)
        m_sp_hud = std::make_shared<HeadsUpDisplay>(*m_sp_render_context, m_hud_settings);
}

void AppBase::Release()
{
    META_FUNCTION_TASK();
    m_sp_logo_badge.reset();
    m_sp_hud.reset();
}

bool AppBase::Resize(const gfx::FrameSize& frame_size, bool)
{
    META_FUNCTION_TASK();
    if (m_frame_size == frame_size)
        return false;

    m_frame_size = frame_size;

    if (m_sp_logo_badge)
        m_sp_logo_badge->FrameResize(frame_size);

    UpdateHelpTextPosition();
    UpdateParametersTextPosition();

    return true;
}

bool AppBase::Update()
{
    META_FUNCTION_TASK();
    if (m_sp_hud && m_app_settings.heads_up_display_mode == IApp::HeadsUpDisplayMode::UserInterface)
        m_sp_hud->Update();

    return true;
}

void AppBase::RenderOverlay(gfx::RenderCommandList& cmd_list)
{
    META_FUNCTION_TASK();
    if (m_sp_hud && m_app_settings.heads_up_display_mode == IApp::HeadsUpDisplayMode::UserInterface)
        m_sp_hud->Draw(cmd_list);

    if (m_sp_logo_badge)
        m_sp_logo_badge->Draw(cmd_list);

    if (m_sp_help_text)
        m_sp_help_text->Draw(cmd_list);

    if (m_sp_parameters_text)
        m_sp_parameters_text->Draw(cmd_list);
}

bool AppBase::SetHeadsUpDisplayMode(IApp::HeadsUpDisplayMode heads_up_display_mode)
{
    META_FUNCTION_TASK();
    if (m_app_settings.heads_up_display_mode == heads_up_display_mode)
        return false;

    m_app_settings.heads_up_display_mode = heads_up_display_mode;

    if (m_app_settings.heads_up_display_mode == IApp::HeadsUpDisplayMode::UserInterface && m_sp_render_context)
    {
        m_sp_hud = std::make_shared<HeadsUpDisplay>(*m_sp_render_context, m_hud_settings);
    }
    else
    {
        m_sp_hud.reset();
    }
    return true;
}

bool AppBase::SetHelpText(const std::string& help_str)
{
    META_FUNCTION_TASK();
    if (!UpdateText(m_sp_help_text, help_str))
        return false;

    UpdateHelpTextPosition();
    return true;
}

bool AppBase::SetParametersText(const std::string& parameters_str)
{
    META_FUNCTION_TASK();
    if (!UpdateText(m_sp_parameters_text, parameters_str))
        return false;

    UpdateParametersTextPosition();
    return true;
}

bool AppBase::UpdateText(Ptr<Text>& sp_text, const std::string& help_str)
{
    if ((!sp_text && help_str.empty()) ||
        (sp_text && sp_text->GetTextUtf8() == help_str))
        return false;

    if (help_str.empty())
    {
        sp_text.reset();
        return true;
    }

    if (!m_sp_render_context)
        throw std::logic_error("Help text can not be initialized without render context.");

    if (sp_text)
    {
        sp_text->SetTextInScreenRect(help_str, {}, true);
    }
    else
    {
        sp_text = std::make_shared<Text>(*m_sp_render_context, GetMainFont(),
             Text::SettingsUtf8
             {
                 "Help",
                 help_str,
                 gfx::FrameRect{}, true,
                 m_app_settings.text_color
             }
        );
    }

    return true;
}

void AppBase::UpdateHelpTextPosition()
{
    META_FUNCTION_TASK();
    if (!m_sp_help_text)
        return;

    // Help text is located in bottom-left corner
    m_sp_help_text->SetScreenOrigin(
        { m_text_margins.width, m_frame_size.height - m_sp_help_text->GetViewport().size.height - m_text_margins.height },
        true
    );
}

void AppBase::UpdateParametersTextPosition()
{
    META_FUNCTION_TASK();
    if (!m_sp_parameters_text)
        return;

    // Parameters text is located in bottom-right corner
    m_sp_parameters_text->SetScreenOrigin(
        {
            m_frame_size.width  - m_sp_parameters_text->GetViewport().size.width  - m_text_margins.width,
            m_frame_size.height - m_sp_parameters_text->GetViewport().size.height - m_text_margins.height
        },
        true
    );
}

Font& AppBase::GetMainFont()
{
    META_FUNCTION_TASK();
    if (m_sp_main_font)
        return *m_sp_main_font;

    if (!m_sp_render_context)
        throw std::logic_error("Main font can not be initialized without render context.");

    m_sp_main_font = Font::Library::Get().GetFont(
        Data::FontProvider::Get(),
        Font::Settings
        {
            "Main", "Fonts/Roboto/Roboto-Regular.ttf", 12u,
            m_sp_render_context->GetFontResolutionDPI(), Font::GetAlphabetDefault()
        }
    ).GetPtr();
    return *m_sp_main_font;
}

} // namespace Methane::UserInterface
