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

FILE: Methane/UserInterface/AppBase.cpp
Base implementation of the Methane user interface application.

******************************************************************************/

#include <Methane/UserInterface/AppBase.h>
#include <Methane/UserInterface/Context.h>
#include <Methane/UserInterface/Text.h>
#include <Methane/UserInterface/Panel.h>
#include <Methane/UserInterface/Badge.h>
#include <Methane/Graphics/RHI/ICommandList.h>
#include <Methane/Graphics/ImageLoader.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

#include <string_view>

namespace Methane::UserInterface
{

static std::vector<size_t> GetLineBreakPositions(std::string_view text_str)
{
    std::vector<size_t> line_break_positions;
    for (size_t line_break_pos = text_str.find('\n', 0);
                line_break_pos != std::string::npos;
                line_break_pos = text_str.find('\n', line_break_pos + 1))
    {
        line_break_positions.emplace_back(line_break_pos);
    }
    return line_break_positions;
}

static void SplitTextToColumns(std::string_view text_str, std::string& left_column_str, std::string& right_column_str)
{
    const std::vector<size_t> line_break_positions = GetLineBreakPositions(text_str);
    if (line_break_positions.empty())
    {
        left_column_str  = text_str;
        right_column_str = std::string();
        return;
    }

    const size_t middle_line_break_position = line_break_positions[line_break_positions.size() / 2];
    left_column_str  = text_str.substr(0, middle_line_break_position);
    right_column_str = text_str.substr(middle_line_break_position + 1);
};

AppBase::AppBase(const IApp::Settings& ui_app_settings)
    : m_app_settings(ui_app_settings)
{
    META_FUNCTION_TASK();
    m_help_columns.first.text_name  = "Help Left";
    m_help_columns.second.text_name = "Help Right";
    m_parameters.text_name          = "Parameters";
}

AppBase::~AppBase()
{
    META_FUNCTION_TASK();

    // Clear static signleton fonts library to guarantee atlas textures destruction before descriptor heaps release
    Font::Library::Get().Clear();
}

void AppBase::InitUI(const Platform::IApp& app, rhi::ICommandQueue& render_cmd_queue, rhi::IRenderPattern& render_pattern, const gfx::FrameSize& frame_size)
{
    META_FUNCTION_TASK();
    m_ui_context_ptr = std::make_unique<Context>(app, render_cmd_queue, render_pattern);
    m_frame_size     = UnitSize(Units::Pixels, frame_size);
    m_text_margins   = m_ui_context_ptr->ConvertTo<Units::Pixels>(m_app_settings.text_margins);
    m_window_padding = m_ui_context_ptr->ConvertTo<Units::Pixels>(m_app_settings.window_padding);

    // Create Methane logo badge
    if (m_app_settings.logo_badge_visible)
    {
        Badge::Settings logo_badge_settings { "Methane Logo" };
        logo_badge_settings.blend_color = m_app_settings.logo_badge_color;
        m_logo_badge_ptr = std::make_shared<Badge>(
            *m_ui_context_ptr, Data::TextureProvider::Get(), "MethaneLogoNameWatermark.png", std::move(logo_badge_settings)
        );
    }

    // Create heads-up-display (HUD)
    m_app_settings.hud_settings.position = m_app_settings.window_padding;
    if (m_app_settings.heads_up_display_mode == HeadsUpDisplayMode::UserInterface)
    {
        m_hud_ptr = std::make_shared<HeadsUpDisplay>(*m_ui_context_ptr, Data::FontProvider::Get(), m_app_settings.hud_settings);
    }

    // Update displayed text blocks
    if (!m_help_columns.first.text_str.empty()  && UpdateTextItem(m_help_columns.first) &&
        (m_help_columns.second.text_str.empty() || UpdateTextItem(m_help_columns.second)))
    {
        UpdateHelpTextPosition();
    }

    if (!m_parameters.text_str.empty() && UpdateTextItem(m_parameters))
    {
        UpdateParametersTextPosition();
    }
}

void AppBase::ReleaseUI()
{
    META_FUNCTION_TASK();
    m_logo_badge_ptr.reset();
    m_hud_ptr.reset();
    m_main_font_ptr.reset();
    m_help_columns.first.Reset(false);
    m_help_columns.second.Reset(false);
    m_parameters.Reset(false);
    m_ui_context_ptr.reset();
}

bool AppBase::ResizeUI(const gfx::FrameSize& frame_size, bool)
{
    META_FUNCTION_TASK();
    const UnitSize frame_size_px(Units::Pixels, frame_size);
    if (m_frame_size == frame_size_px)
        return false;

    m_frame_size = frame_size_px;

    if (m_logo_badge_ptr)
        m_logo_badge_ptr->FrameResize(frame_size_px);

    UpdateHelpTextPosition();
    UpdateParametersTextPosition();

    return true;
}

bool AppBase::UpdateUI() const
{
    META_FUNCTION_TASK();
    if (m_hud_ptr && m_app_settings.heads_up_display_mode == HeadsUpDisplayMode::UserInterface)
        m_hud_ptr->Update(m_frame_size);

    m_help_columns.first.Update(m_frame_size);
    m_help_columns.second.Update(m_frame_size);
    m_parameters.Update(m_frame_size);
    return true;
}

void AppBase::RenderOverlay(rhi::IRenderCommandList& cmd_list) const
{
    META_FUNCTION_TASK();
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Overlay Rendering");

    if (m_hud_ptr && m_app_settings.heads_up_display_mode == HeadsUpDisplayMode::UserInterface)
        m_hud_ptr->Draw(cmd_list, s_debug_group.get());

    m_help_columns.first.Draw(cmd_list, s_debug_group.get());
    m_help_columns.second.Draw(cmd_list, s_debug_group.get());
    m_parameters.Draw(cmd_list, s_debug_group.get());

    if (m_logo_badge_ptr)
        m_logo_badge_ptr->Draw(cmd_list, s_debug_group.get());
}

void AppBase::TextItem::Update(const FrameSize& frame_size) const
{
    META_FUNCTION_TASK();
    if (text_ptr)
    {
        text_ptr->Update(frame_size);
    }
}

void AppBase::TextItem::Draw(rhi::IRenderCommandList& cmd_list, rhi::ICommandListDebugGroup* p_debug_group) const
{
    META_FUNCTION_TASK();
    if (panel_ptr)
    {
        panel_ptr->Draw(cmd_list, p_debug_group);
    }
    if (text_ptr)
    {
        text_ptr->Draw(cmd_list, p_debug_group);
    }
}

void AppBase::TextItem::Reset(bool forget_text_string)
{
    META_FUNCTION_TASK();

    text_ptr.reset();
    panel_ptr.reset();

    if (forget_text_string)
        text_str.clear();
}

bool AppBase::SetHeadsUpDisplayUIMode(HeadsUpDisplayMode heads_up_display_mode)
{
    META_FUNCTION_TASK();
    if (m_app_settings.heads_up_display_mode == heads_up_display_mode)
        return false;

    m_app_settings.heads_up_display_mode = heads_up_display_mode;

    // Wait for all in-flight rendering to complete before creating and releasing GPU resources
    m_ui_context_ptr->GetRenderContext().WaitForGpu(rhi::IRenderContext::WaitFor::RenderComplete);

    if (m_app_settings.heads_up_display_mode == HeadsUpDisplayMode::UserInterface && m_ui_context_ptr)
    {
        m_hud_ptr = std::make_shared<HeadsUpDisplay>(*m_ui_context_ptr, Data::FontProvider::Get(), m_app_settings.hud_settings);
    }
    else
    {
        m_hud_ptr.reset();
        Font::Library::Get().RemoveFont(m_app_settings.hud_settings.major_font.name);
        Font::Library::Get().RemoveFont(m_app_settings.hud_settings.minor_font.name);
    }
    return true;
}

bool AppBase::SetHelpText(std::string_view help_str)
{
    META_FUNCTION_TASK();
    if (m_help_text_str == help_str)
        return false;

    // Wait for all in-flight rendering to complete before creating and releasing GPU resources
    m_ui_context_ptr->GetRenderContext().WaitForGpu(rhi::IRenderContext::WaitFor::RenderComplete);

    m_help_text_str = help_str;
    m_help_columns.first.text_str = help_str;

    if (!UpdateTextItem(m_help_columns.first))
    {
        m_help_columns.second.Reset(true);
        return false;
    }

    // Split help text into two columns
    // when single column does not fit into half of window height
    // and estimated width of two columns first in 2/3 of window width
    if (const gfx::FrameSize single_column_size = m_help_columns.first.text_ptr->GetRectInPixels().size;
        single_column_size.GetHeight() + m_window_padding.GetY() > m_frame_size.GetHeight() / 2 &&
        single_column_size.GetWidth() < m_frame_size.GetWidth() / 2)
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
        m_help_columns.second.Reset(true);
    }

    UpdateHelpTextPosition();
    return true;
}

bool AppBase::SetParametersText(std::string_view parameters_str)
{
    META_FUNCTION_TASK();
    if (m_parameters.text_str == parameters_str)
        return false;

    m_parameters.text_str = parameters_str;

    if (!m_ui_context_ptr)
        return true;

    // Wait for all in-flight rendering to complete before creating and releasing GPU resources
    m_ui_context_ptr->GetRenderContext().WaitForGpu(rhi::IRenderContext::WaitFor::RenderComplete);

    if (!UpdateTextItem(m_parameters))
        return false;

    UpdateParametersTextPosition();
    return true;
}

bool AppBase::UpdateTextItem(TextItem& item)
{
    if ((!item.text_ptr && item.text_str.empty()) ||
        (item.text_ptr  && item.text_ptr->GetTextUtf8() == item.text_str))
        return false;

    if (item.text_str.empty())
    {
        item.Reset(true);

        // If main font is hold only by this class and Font::Library, then it can be removed as unused
        if (m_main_font_ptr.use_count() == 2)
        {
            Font::Library::Get().RemoveFont(m_app_settings.main_font.name);
            m_main_font_ptr.reset();
        }
        return false;
    }

    META_CHECK_ARG_NOT_NULL_DESCR(m_ui_context_ptr, "help text can not be initialized without render context");

    if (!item.panel_ptr)
    {
        item.panel_ptr = std::make_shared<Panel>(*m_ui_context_ptr, UnitRect(),
            Panel::Settings
            {
                item.text_name + " Panel"
            }
        );
    }

    if (item.text_ptr)
    {
        item.text_ptr->SetText(item.text_str);
    }
    else
    {
        item.text_ptr = std::make_shared<Text>(*m_ui_context_ptr, GetMainFont(),
             Text::SettingsUtf8
             {
                 item.text_name,
                 item.text_str,
                 UnitRect(m_app_settings.text_margins),
                 Text::Layout{ Text::Wrap::None },
                 m_app_settings.text_color
             }
        );
        item.panel_ptr->AddChild(*item.text_ptr);
    }

    return true;
}

void AppBase::UpdateHelpTextPosition() const
{
    META_FUNCTION_TASK();
    if (!m_help_columns.first.panel_ptr)
        return;

    // Help text columns are located in bottom-left corner
    const FrameSize& first_text_size = m_help_columns.first.text_ptr->GetRectInPixels().size;
    m_help_columns.first.panel_ptr->SetRect(UnitRect(
        Units::Pixels,
        FramePoint(m_window_padding.GetX(), m_frame_size.GetHeight() - first_text_size.GetHeight() - m_window_padding.GetY() * 3),
        first_text_size + static_cast<FrameSize>(m_window_padding * 2)
    ));

    if (!m_help_columns.second.panel_ptr)
        return;

    const FrameSize& second_text_size = m_help_columns.first.text_ptr->GetRectInPixels().size;
    const UnitRect&  first_panel_rect = m_help_columns.first.panel_ptr->GetRectInPixels();
    m_help_columns.second.panel_ptr->SetRect(UnitRect(
        Units::Pixels,
        FramePoint(first_panel_rect.GetRight() + m_window_padding.GetX(), first_panel_rect.GetTop()),
        second_text_size + static_cast<FrameSize>(m_window_padding * 2)
    ));
}

void AppBase::UpdateParametersTextPosition() const
{
    META_FUNCTION_TASK();
    if (!m_parameters.text_ptr)
        return;

    // Parameters text is located in bottom-right corner
    const FrameSize  window_padding_size(m_window_padding);
    const FrameSize& parameters_text_size = m_parameters.text_ptr->GetRectInPixels().size;
    m_parameters.panel_ptr->SetRect(UnitRect(
        Units::Pixels,
        FramePoint(m_frame_size.GetWidth()  - parameters_text_size.GetWidth()  - window_padding_size.GetWidth()  * 3,
                   m_frame_size.GetHeight() - parameters_text_size.GetHeight() - window_padding_size.GetHeight() * 3),
        parameters_text_size + window_padding_size * 2U
    ));
}

Font& AppBase::GetMainFont()
{
    META_FUNCTION_TASK();
    if (m_main_font_ptr)
        return *m_main_font_ptr;

    META_CHECK_ARG_NOT_NULL_DESCR(m_ui_context_ptr, "main font can not be initialized without render context");
    m_main_font_ptr = Font::Library::Get().GetFont(
        Data::FontProvider::Get(),
        Font::Settings{ m_app_settings.main_font, m_ui_context_ptr->GetFontResolutionDpi(), Font::GetAlphabetDefault() }
    ).GetPtr();
    return *m_main_font_ptr;
}

const Data::IProvider& AppBase::GetFontProvider() const noexcept
{
    META_FUNCTION_TASK();
    return Data::FontProvider::Get();
}

} // namespace Methane::UserInterface
