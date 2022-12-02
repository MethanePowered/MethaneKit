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
#include <xcb/randr.h>

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

std::pair<Input::Mouse::Button, int> ConvertXcbMouseButton(xcb_button_t button)
{
    META_FUNCTION_TASK();
    switch(button)
    {
    case XCB_BUTTON_INDEX_1:     return { Input::Mouse::Button::Left, 0 };
    case XCB_BUTTON_INDEX_2:     return { Input::Mouse::Button::Middle, 0 };
    case XCB_BUTTON_INDEX_3:     return { Input::Mouse::Button::Right, 0 };
    case XCB_BUTTON_INDEX_4:     return { Input::Mouse::Button::VScroll, 1 };
    case XCB_BUTTON_INDEX_5:     return { Input::Mouse::Button::VScroll, -1 };
    case XCB_BUTTON_INDEX_5 + 1: return { Input::Mouse::Button::HScroll, 1 };
    case XCB_BUTTON_INDEX_5 + 2: return { Input::Mouse::Button::HScroll, -1 };
    default: META_UNEXPECTED_ARG_DESCR(button, "XCB mouse button is not supported");
    }
}

Input::Keyboard::Key ConvertXcbKey(_XDisplay* display, xcb_window_t window, xcb_keycode_t key_detail, uint16_t key_state)
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
        const Input::Keyboard::Key key = Input::Keyboard::KeyConverter({ key_sym, key_state }).GetKey();
        if (key != Input::Keyboard::Key::Unknown)
            return key;
    }

    return Input::Keyboard::Key::Unknown;
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

ScreenRect GetPrimaryMonitorRect(xcb_connection_t* connection, xcb_window_t root)
{
    META_FUNCTION_TASK();
    xcb_generic_error_t* error = nullptr;
    ScreenRect screen_rect {};

#if 0 // Use X11 screen resources

    const xcb_randr_get_screen_resources_current_cookie_t screen_resources_cookie = xcb_randr_get_screen_resources_current(connection, root);
    xcb_randr_get_screen_resources_current_reply_t* screen_resources_reply  = xcb_randr_get_screen_resources_current_reply(connection, screen_resources_cookie, &error);
    if (error)
        throw XcbException("failed to get screen outputs", *error);

    const xcb_timestamp_t timestamp = screen_resources_reply->config_timestamp;
    const int screen_outputs_length = xcb_randr_get_screen_resources_current_outputs_length(screen_resources_reply);
    const xcb_randr_output_t* screen_outputs = xcb_randr_get_screen_resources_current_outputs(screen_resources_reply);

    for (int output_index = 0; output_index < screen_outputs_length; ++output_index)
    {
        const xcb_randr_get_output_info_cookie_t output_info_cookie = xcb_randr_get_output_info(connection, screen_outputs[output_index], timestamp);
        xcb_randr_get_output_info_reply_t* output = xcb_randr_get_output_info_reply(connection, output_info_cookie, &error);
        if (error)
            throw XcbException("failed to get screen output information", *error);

        if (!output || output->crtc == XCB_NONE || output->connection == XCB_RANDR_CONNECTION_DISCONNECTED)
            continue;

        const xcb_randr_get_crtc_info_cookie_t crtc_info_cookie = xcb_randr_get_crtc_info(connection, output->crtc, timestamp);
        xcb_randr_get_crtc_info_reply_t* crtc = xcb_randr_get_crtc_info_reply(connection, crtc_info_cookie, &error);
        if (error)
            throw XcbException("failed to get screen output information", *error);

        screen_rect.x      = crtc->x;
        screen_rect.y      = crtc->y;
        screen_rect.width  = crtc->width;
        screen_rect.height = crtc->height;

        free(crtc);
        free(output);

        break; // Use first found screen as primary
    }

    free(screen_resources_reply);
    return screen_rect;

#else // Use X11 monitors

    const xcb_randr_get_monitors_cookie_t monitors_cookie = xcb_randr_get_monitors(connection, root, 1);
    xcb_randr_get_monitors_reply_t* monitors_reply = xcb_randr_get_monitors_reply(connection, monitors_cookie, &error);
    if (error)
        throw XcbException("failed to get monitors", *error);

    for (xcb_randr_monitor_info_iterator_t monitors_iter = xcb_randr_get_monitors_monitors_iterator(monitors_reply);
         monitors_iter.rem > 0;
         xcb_randr_monitor_info_next(&monitors_iter))
    {
        const xcb_randr_monitor_info_t* const monitor_info = monitors_iter.data;

        screen_rect.x      = monitor_info->x;
        screen_rect.y      = monitor_info->y;
        screen_rect.width  = monitor_info->width;
        screen_rect.height = monitor_info->height;

        if (monitor_info->primary)
            return screen_rect;
    }

    free(monitors_reply); // NOSONAR
    return screen_rect;

#endif
}

} // namespace Methane::Platform
;