/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Methane/Platform/Linux/XcbUtils.h
X11/XCB utility functions.

******************************************************************************/

#include "XcbUtils.h"

#include <fmt/format.h>

#include <X11/Xlib-xcb.h>
#include <X11/keysym.h>

namespace Methane::Platform::Linux
{

XcbException::XcbException(std::string_view error_message, const xcb_generic_error_t& error)
    : std::runtime_error(fmt::format("X11/XCB error: {}, error code {}", error_message, error.error_code))
    , m_error(error)
{ }

uint32_t PackXcbColor(const RgbColor& color)
{
    META_FUNCTION_TASK();
    return static_cast<uint32_t>(color.r) << 16 |
           static_cast<uint32_t>(color.g) << 8 |
           static_cast<uint32_t>(color.b);
}

uint32_t GetXcbSystemColor(SystemColor color_type)
{
    META_FUNCTION_TASK();
    static const std::array<RgbColor, 11> s_system_colors{{ // Ubuntu 20.04 dark theme
        { 52, 52, 52 },     // Background
        { 247, 247, 247 },  // DefaultText
        { 34, 34, 34 },     // ButtonBorderNormal
        { 179, 106, 80 },   // ButtonBorderSelected
        { 72, 72, 72 },     // ButtonBackgroundNormal
        { 78, 78, 78 },     // ButtonBackgroundHovered
        { 162, 18, 35 },    // ButtonBackgroundError
        { 171, 19, 37 },    // ButtonBackgroundErrorHovered
        { 208, 139, 0 },    // ButtonBackgroundWarning,
        { 209, 158, 56 },   // ButtonBackgroundWarningHovered,
        { 39, 39, 39 },     // ButtonBackgroundPressed
    }};
    return PackXcbColor(s_system_colors[static_cast<size_t>(color_type)]);
}

std::pair<Mouse::Button, int> ConvertXcbMouseButton(xcb_button_t button)
{
    META_FUNCTION_TASK();
    switch(button)
    {
    case XCB_BUTTON_INDEX_1: return { Mouse::Button::Left, 0 };
    case XCB_BUTTON_INDEX_2: return { Mouse::Button::Middle, 0 };
    case XCB_BUTTON_INDEX_3: return { Mouse::Button::Right, 0 };
    case XCB_BUTTON_INDEX_4: return { Mouse::Button::VScroll, 1 };
    case XCB_BUTTON_INDEX_5: return { Mouse::Button::VScroll, -1 };
    case XCB_BUTTON_INDEX_5 + 1: return { Mouse::Button::HScroll, 1 };
    case XCB_BUTTON_INDEX_5 + 2: return { Mouse::Button::HScroll, -1 };
    default: META_UNEXPECTED_ARG_DESCR(button, "XCB mouse button is not supported");
    }
}

Keyboard::Key ConvertXcbKey(_XDisplay* display, xcb_window_t window, xcb_keycode_t key_detail, uint16_t key_state)
{
    META_FUNCTION_TASK();
    XKeyEvent x_key_event{ 0 };
    x_key_event.display = display;
    x_key_event.window = window;
    x_key_event.state = key_state;
    x_key_event.keycode = key_detail;

    for (int i = 0; i < 4; ++i)
    {
        const KeySym key_sym = XLookupKeysym(&x_key_event, i);
        const Keyboard::Key key = Keyboard::KeyConverter({ key_sym, key_state }).GetKey();
        if (key != Keyboard::Key::Unknown)
            return key;
    }

    return Keyboard::Key::Unknown;
}

void XcbCheck(xcb_void_cookie_t cookie, xcb_connection_t* connection, std::string_view error_message)
{
    META_FUNCTION_TASK();
    const xcb_generic_error_t* error = xcb_request_check(connection, cookie);
    if (error)
    {
        xcb_disconnect(connection);
        throw XcbException(error_message, *error);
    }
}

void XcbMeasureText(xcb_connection_t* connection, xcb_font_t font, std::string_view text, uint32_t& width, uint32_t& height, uint32_t& ascent)
{
    META_FUNCTION_TASK();
    std::vector<xcb_char2b_t> xcb_str;
    xcb_str.reserve(text.length());
    std::transform(text.begin(), text.end(), std::back_inserter(xcb_str),
                   [](const char c) { return xcb_char2b_t{ 0, static_cast<uint8_t>(c) }; });

    const xcb_query_text_extents_cookie_t cookie = xcb_query_text_extents(connection, font, static_cast<uint32_t>(xcb_str.size()), xcb_str.data());
    xcb_query_text_extents_reply_t* reply = xcb_query_text_extents_reply(connection, cookie, nullptr);
    META_CHECK_ARG_NOT_NULL_DESCR(reply, "failed to query XCB text extents");

    width  = reply->overall_width;
    height = reply->font_ascent + reply->font_descent;
    ascent = reply->font_ascent;
    free(reply); // NOSONAR
}

xcb_intern_atom_reply_t* GetXcbInternAtomReply(xcb_connection_t* connection, std::string_view name) noexcept
{
    META_FUNCTION_TASK();
    const xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, false, static_cast<uint16_t>(name.length()), name.data());
    return xcb_intern_atom_reply(connection, cookie, nullptr);
}

xcb_atom_t GetXcbInternAtom(xcb_connection_t* xcb_connection, std::string_view name) noexcept
{
    META_FUNCTION_TASK();
    xcb_intern_atom_reply_t* atom_reply = GetXcbInternAtomReply(xcb_connection, name);
    const xcb_atom_t atom = atom_reply ? atom_reply->atom : static_cast<xcb_atom_t>(XCB_ATOM_NONE);
    free(atom_reply); // NOSONAR
    return atom;
}

void SetXcbWindowStringProperty(xcb_connection_t* connection, xcb_window_t window, xcb_atom_enum_t property_id, const std::string_view& value)
{
    META_FUNCTION_TASK();
    XcbCheck(xcb_change_property_checked(connection, XCB_PROP_MODE_REPLACE, window, property_id, XCB_ATOM_STRING, 8, static_cast<uint32_t>(value.size()), value.data()),
             connection, "failed to set string property");
}

} // namespace Methane::Platform
