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

#include <Methane/UserInterface/Context.h>
#include <Methane/UserInterface/Text.h>
#include <Methane/UserInterface/Panel.h>
#include <Methane/UserInterface/Badge.h>
#include <Methane/Graphics/ImageLoader.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

namespace Methane::UserInterface
{

static const Font::Description g_major_font_desc = { "Major", "Fonts/RobotoMono/RobotoMono-Bold.ttf",    18u };
static const Font::Description g_main_font_desc  = { "Main",  "Fonts/RobotoMono/RobotoMono-Regular.ttf", 10u };

static std::vector<size_t> GetLineBreakPositions(const std::string& text_str)
{
    std::vector<size_t> line_break_positions;
    for (size_t line_break_pos = text_str.find('\n', 0);
                line_break_pos != std::string::npos;
                line_break_pos = text_str.find('\n', line_break_positions.back() + 1))
    {
        line_break_positions.emplace_back(line_break_pos);
    }
    return line_break_positions;
}

static void SplitTextToColumns(const std::string& text_str, std::string& left_column_str, std::string& right_column_str)
{
    const std::vector<size_t> line_break_positions = GetLineBreakPositions(text_str);
    if (line_break_positions.empty())
    {
        left_column_str = text_str;
        right_column_str.clear();
        return;
    }

    const size_t middle_line_break_position = line_break_positions[line_break_positions.size() / 2];
    left_column_str = text_str.substr(0, middle_line_break_position);
    right_column_str = text_str.substr(middle_line_break_position + 1);
};

AppBase::AppBase(const IApp::Settings& ui_app_settings)
    : m_app_settings(ui_app_settings)
    , m_hud_settings({
        g_major_font_desc, ui_app_settings.text_margins
    })
{
    META_FUNCTION_TASK();
    m_hud_settings.text_color = m_app_settings.text_color;
    m_help_columns.first.text_name  = "Help Left";
    m_help_columns.second.text_name = "Help Right";
    m_parameters.text_name          = "Parameters";
}

AppBase::~AppBase() = default;

void AppBase::Init(gfx::RenderContext& render_context, const gfx::FrameSize& frame_size)
{
    META_FUNCTION_TASK();
    m_sp_ui_context = std::make_unique<Context>(render_context);
    m_frame_size    = UnitSize(frame_size, Units::Pixels);
    m_text_margins  = m_sp_ui_context->ConvertToPixels(m_app_settings.text_margins);

    // Create Methane logo badge
    if (m_app_settings.show_logo_badge)
    {
        Badge::Settings logo_badge_settings;
        logo_badge_settings.blend_color = m_app_settings.logo_badge_color;
        m_sp_logo_badge = std::make_shared<Badge>(
            *m_sp_ui_context, Data::TextureProvider::Get(), "Logo/MethaneLogoNameWatermark.png", std::move(logo_badge_settings)
        );
    }

    // Create heads-up-display (HUD)
    m_hud_settings.position = m_app_settings.text_margins;
    if (m_app_settings.heads_up_display_mode == IApp::HeadsUpDisplayMode::UserInterface)
    {
        m_sp_hud = std::make_shared<HeadsUpDisplay>(*m_sp_ui_context, Data::FontProvider::Get(), m_hud_settings);
    }

    // Update displayed text blocks
    if (!m_help_columns.first.text_str.empty() && UpdateTextItem(m_help_columns.first) &&
        (m_help_columns.second.text_str.empty() || UpdateTextItem(m_help_columns.second)))
    {
        UpdateHelpTextPosition();
    }

    if (!m_parameters.text_str.empty() && UpdateTextItem(m_parameters))
    {
        UpdateParametersTextPosition();
    }
}

void AppBase::Release()
{
    META_FUNCTION_TASK();
    m_sp_logo_badge.reset();
    m_sp_hud.reset();
    m_sp_main_font.reset();
    m_help_columns.first.sp_text.reset();
    m_help_columns.second.sp_text.reset();
    m_parameters.sp_text.reset();
    m_sp_ui_context.reset();
}

bool AppBase::Resize(const gfx::FrameSize& frame_size, bool)
{
    META_FUNCTION_TASK();
    if (m_frame_size == frame_size)
        return false;

    m_frame_size = UnitSize(frame_size, Units::Pixels);

    if (m_sp_logo_badge)
        m_sp_logo_badge->FrameResize(UnitSize(frame_size));

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

    m_help_columns.first.Draw(cmd_list);
    m_help_columns.second.Draw(cmd_list);
    m_parameters.Draw(cmd_list);

    if (m_sp_logo_badge)
        m_sp_logo_badge->Draw(cmd_list);
}

void AppBase::TextItem::Draw(gfx::RenderCommandList& cmd_list)
{
    META_FUNCTION_TASK();
    if (sp_panel)
    {
        sp_panel->Draw(cmd_list);
    }
    if (sp_text)
    {
        sp_text->Draw(cmd_list);
    }
}

void AppBase::TextItem::Reset()
{
    META_FUNCTION_TASK();
    text_str.clear();
    sp_text.reset();
    sp_panel.reset();
}


bool AppBase::SetHeadsUpDisplayMode(IApp::HeadsUpDisplayMode heads_up_display_mode)
{
    META_FUNCTION_TASK();
    if (m_app_settings.heads_up_display_mode == heads_up_display_mode)
        return false;

    m_app_settings.heads_up_display_mode = heads_up_display_mode;

    if (m_app_settings.heads_up_display_mode == IApp::HeadsUpDisplayMode::UserInterface && m_sp_ui_context)
    {
        m_sp_hud = std::make_shared<HeadsUpDisplay>(*m_sp_ui_context, Data::FontProvider::Get(), m_hud_settings);
    }
    else
    {
        m_sp_hud.reset();
        Font::Library::Get().RemoveFont(m_hud_settings.major_font.name);
    }
    return true;
}

bool AppBase::SetHelpText(const std::string& help_str)
{
    META_FUNCTION_TASK();
    if (m_help_text_str == help_str)
        return false;

    m_help_text_str = help_str;
    m_help_columns.first.text_str = help_str;

    if (!UpdateTextItem(m_help_columns.first))
    {
        m_help_columns.second.Reset();
        return false;
    }

    // Split help text into two columns
    // when single column does not fit into half of window height
    // and estimated width of two columns first in 2/3 of window width
    const gfx::FrameSize single_column_size = m_help_columns.first.sp_text->GetViewport().size;
    if (single_column_size.height + m_text_margins.GetY() > m_frame_size.height / 2 &&
        single_column_size.width < m_frame_size.width / 2)
    {
        SplitTextToColumns(m_help_text_str, m_help_columns.first.text_str, m_help_columns.second.text_str);
        if (!m_help_columns.second.text_str.empty())
        {
            UpdateTextItem(m_help_columns.first);
            UpdateTextItem(m_help_columns.second);
        }
    }
    else
    {
        m_help_columns.first.text_str = m_help_text_str;
        m_help_columns.second.Reset();
    }

    UpdateHelpTextPosition();
    return true;
}

bool AppBase::SetParametersText(const std::string& parameters_str)
{
    META_FUNCTION_TASK();
    if (m_parameters.text_str == parameters_str)
        return false;

    m_parameters.text_str = parameters_str;

    if (!UpdateTextItem(m_parameters))
        return false;

    UpdateParametersTextPosition();
    return true;
}

bool AppBase::UpdateTextItem(TextItem& item)
{
    if ((!item.sp_text && item.text_str.empty()) ||
        (item.sp_text && item.sp_text->GetTextUtf8() == item.text_str))
        return false;

    if (item.text_str.empty())
    {
        item.Reset();

        // If main font is hold only by this class and Font::Library, then it can be removed as unused
        if (m_sp_main_font.use_count() == 2)
        {
            Font::Library::Get().RemoveFont(g_main_font_desc.name);
            m_sp_main_font.reset();
        }
        return false;
    }

    if (!m_sp_ui_context)
        throw std::logic_error("Help text can not be initialized without render context.");

    if (!item.sp_panel)
    {
        item.sp_panel = std::make_shared<Panel>(*m_sp_ui_context, UnitRect(),
            Panel::Settings
            {
                item.text_name + " Panel"
            }
        );
    }

    if (item.sp_text)
    {
        item.sp_text->SetTextInScreenRect(item.text_str, {});
    }
    else
    {
        item.sp_text = std::make_shared<Text>(*m_sp_ui_context, GetMainFont(),
             Text::SettingsUtf8
             {
                 item.text_name,
                 item.text_str,
                 UnitRect{},
                 m_app_settings.text_color,
                 Text::Wrap::None
             }
        );
        item.sp_panel->AddChild(*item.sp_text);
    }

    item.sp_panel->SetRect(item.sp_text->GetRectInPixels());
    return true;
}

void AppBase::UpdateHelpTextPosition()
{
    META_FUNCTION_TASK();
    if (!m_help_columns.first.sp_text)
        return;

    // Help text columns are located in bottom-left corner
    const gfx::FrameSize& first_column_size = m_help_columns.first.sp_text->GetViewportInPixels().size;
    m_help_columns.first.sp_text->SetOrigin(UnitPoint(
        m_text_margins.GetX(),
        m_frame_size.height - first_column_size.height - m_text_margins.GetY(),
        Units::Pixels
    ));

    if (m_help_columns.first.sp_panel)
        m_help_columns.first.sp_panel->SetRect(m_help_columns.first.sp_text->GetRectInPixels());

    if (!m_help_columns.second.sp_text)
        return;

    const gfx::FrameSize& second_column_size = m_help_columns.first.sp_text->GetViewportInPixels().size;
    m_help_columns.second.sp_text->SetOrigin(UnitPoint(
        first_column_size.width + 2 * m_text_margins.GetX(),
        m_frame_size.height - second_column_size.height - m_text_margins.GetY(),
        Units::Pixels
    ));

    if (m_help_columns.second.sp_panel)
        m_help_columns.second.sp_panel->SetRect(m_help_columns.second.sp_text->GetRectInPixels());
}

void AppBase::UpdateParametersTextPosition()
{
    META_FUNCTION_TASK();
    if (!m_parameters.sp_text)
        return;

    // Parameters text is located in bottom-right corner
    const gfx::FrameSize& parameters_size = m_parameters.sp_text->GetViewportInPixels().size;
    m_parameters.sp_text->SetOrigin(UnitPoint(
        m_frame_size.width  - parameters_size.width  - m_text_margins.GetX(),
        m_frame_size.height - parameters_size.height - m_text_margins.GetY(),
        Units::Pixels
    ));

    if (m_parameters.sp_panel)
        m_parameters.sp_panel->SetRect(m_parameters.sp_text->GetRectInPixels());
}

Font& AppBase::GetMainFont()
{
    META_FUNCTION_TASK();
    if (m_sp_main_font)
        return *m_sp_main_font;

    if (!m_sp_ui_context)
        throw std::logic_error("Main font can not be initialized without render context.");

    m_sp_main_font = Font::Library::Get().GetFont(
        Data::FontProvider::Get(),
        Font::Settings{ g_main_font_desc, m_sp_ui_context->GetFontResolutionDpi(), Font::GetAlphabetDefault() }
    ).GetPtr();
    return *m_sp_main_font;
}

} // namespace Methane::UserInterface