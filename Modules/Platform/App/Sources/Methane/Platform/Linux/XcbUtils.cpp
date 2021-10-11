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

void XcbCheck(xcb_void_cookie_t cookie, xcb_connection_t *connection, std::string_view error_message)
{
    const xcb_generic_error_t* error = xcb_request_check(connection, cookie);
    if (error)
    {
        xcb_disconnect(connection);
        throw std::runtime_error(fmt::format("X11/XCB error: {}, error code {}", error_message, error->error_code));
    }
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
    XcbCheck(xcb_change_property_checked(connection, XCB_PROP_MODE_REPLACE, window, property_id, XCB_ATOM_STRING, 8, value.size(), value.data()),
             connection, "failed to set string property");
}

} // namespace Methane::Platform
