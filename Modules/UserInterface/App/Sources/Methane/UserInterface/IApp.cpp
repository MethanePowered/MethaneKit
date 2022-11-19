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

FILE: Methane/UserInterface/App.h
Interface of the base user interface application template class defined in App.hpp

******************************************************************************/

#include <Methane/UserInterface/IApp.h>

namespace Methane::UserInterface
{

AppSettings& AppSettings::SetHeadsUpDisplayMode(HeadsUpDisplayMode new_heads_up_display_mode) noexcept
{
    META_FUNCTION_TASK();
    heads_up_display_mode = new_heads_up_display_mode;
    return *this;
}

AppSettings& AppSettings::SetLogoBadgeVisible(bool new_logo_badge_visible) noexcept
{
    META_FUNCTION_TASK();
    logo_badge_visible = new_logo_badge_visible;
    return *this;
}

AppSettings& AppSettings::SetLogoBadgeColor(const Color4F& new_logo_badge_color) noexcept
{
    META_FUNCTION_TASK();
    logo_badge_color = new_logo_badge_color;
    return *this;
}

AppSettings& AppSettings::SetTextColor(const Color4F& new_text_color) noexcept
{
    META_FUNCTION_TASK();
    text_color = new_text_color;
    return *this;
}

AppSettings& AppSettings::SetTextMargins(const UnitPoint& new_text_margings) noexcept
{
    META_FUNCTION_TASK();
    text_margins = new_text_margings;
    return *this;
}

AppSettings& AppSettings::SetWindowPadding(const UnitPoint& new_window_padding) noexcept
{
    META_FUNCTION_TASK();
    window_padding = new_window_padding;
    return *this;
}

AppSettings& AppSettings::SetMainFont(const Font::Description& new_main_font) noexcept
{
    META_FUNCTION_TASK();
    main_font = new_main_font;
    return *this;
}

AppSettings& AppSettings::SetHudSettings(const HeadsUpDisplay::Settings& new_hud_settings) noexcept
{
    META_FUNCTION_TASK();
    hud_settings = new_hud_settings;
    return *this;
}

} // namespace Methane::UserInterface
