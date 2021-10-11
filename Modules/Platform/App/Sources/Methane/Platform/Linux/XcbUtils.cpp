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

namespace Methane::Platform::Linux
{

void XcbCheck(xcb_void_cookie_t cookie, xcb_connection_t* connection, std::string_view error_message)
{
    META_FUNCTION_TASK();
    const xcb_generic_error_t* error = xcb_request_check(connection, cookie);
    if (error)
    {
        xcb_disconnect(connection);
        throw std::runtime_error(fmt::format("X11/XCB error: {}, error code {}", error_message, error->error_code));
    }
}

void XcbMeasureText(xcb_connection_t* connection, xcb_font_t font, std::string_view text, uint32_t& width, uint32_t& height, uint32_t& ascent)
{
    META_FUNCTION_TASK();
    std::vector<xcb_char2b_t> xcb_str;
    xcb_str.reserve(text.length());
    std::transform(text.begin(), text.end(), std::back_inserter(xcb_str),
                   [](const char c) { return xcb_char2b_t{ 0, static_cast<uint8_t>(c) }; });

    const xcb_query_text_extents_cookie_t cookie = xcb_query_text_extents(connection, font, xcb_str.size(), xcb_str.data());
    xcb_query_text_extents_reply_t* reply = xcb_query_text_extents_reply(connection, cookie, NULL);
    META_CHECK_ARG_NOT_NULL_DESCR(reply, "failed to query XCB text extents");

    width  = reply->overall_width;
    height = reply->font_ascent + reply->font_descent;
    ascent = reply->font_ascent;
    free(reply);
}

xcb_intern_atom_reply_t* GetXcbInternAtomReply(xcb_connection_t* connection, std::string_view name) noexcept
{
    META_FUNCTION_TASK();
    const xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, false, name.length(), name.data());
    return xcb_intern_atom_reply(connection, cookie, nullptr);
}

xcb_atom_t GetXcbInternAtom(xcb_connection_t* xcb_connection, std::string_view name) noexcept
{
    META_FUNCTION_TASK();
    xcb_intern_atom_reply_t* atom_reply = GetXcbInternAtomReply(xcb_connection, name);
    const xcb_atom_t atom = atom_reply ? atom_reply->atom : static_cast<xcb_atom_t>(XCB_ATOM_NONE);
    free(atom_reply);
    return atom;
}

void SetXcbWindowStringProperty(xcb_connection_t* connection, xcb_window_t window, xcb_atom_enum_t property_id, const std::string_view& value)
{
    META_FUNCTION_TASK();
    XcbCheck(xcb_change_property_checked(connection, XCB_PROP_MODE_REPLACE, window, property_id, XCB_ATOM_STRING, 8, value.size(), value.data()),
             connection, "failed to set string property");
}

} // namespace Methane::Platform
