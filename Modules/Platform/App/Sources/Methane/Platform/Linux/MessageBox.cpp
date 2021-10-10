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

FILE: Methane/Platform/Linux/MessageBox.cpp
Linux message box implementation with X11/XCB.

******************************************************************************/

#include "XcbUtils.h"

#include <Methane/Platform/Linux/MessageBox.h>
#include <Methane/Platform/Utils.h>
#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

#include <string_view>
#include <optional>

#include <X11/Xlib-xcb.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

namespace Methane::Platform
{

MessageBox::MessageBox(const AppEnvironment& app_env)
    : m_app_env(app_env)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(m_app_env.display, "X11 display should be opened");
    META_CHECK_ARG_NOT_NULL_DESCR(m_app_env.screen, "XCB screen should be initialized");
    META_CHECK_ARG_NOT_NULL_DESCR(m_app_env.connection, "XCB connection should be initialized");

    // Prepare initial window properties
    const uint32_t value_mask = XCB_CW_EVENT_MASK;
    const std::array<uint32_t, 1> values{{
        XCB_EVENT_MASK_EXPOSURE |
        XCB_EVENT_MASK_STRUCTURE_NOTIFY |
        XCB_EVENT_MASK_KEY_RELEASE |
        XCB_EVENT_MASK_KEY_PRESS |
        XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE
    }};

    // Calculate frame size relative to screen_id size in case of floating point value
    const uint16_t frame_width  = 640;
    const uint16_t frame_height = 480;
    const int16_t pos_x = static_cast<int16_t>(m_app_env.screen->width_in_pixels - frame_width) / 2;
    const int16_t pos_y = static_cast<int16_t>(m_app_env.screen->height_in_pixels - frame_height) / 2;

    // Create window and position it in the center of the screen_id
    m_dialog_window = xcb_generate_id(m_app_env.connection);
    xcb_create_window(m_app_env.connection, m_app_env.screen->root_depth,
                      m_dialog_window, m_app_env.screen->root,
                      pos_x, pos_y, frame_width, frame_height, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, m_app_env.screen->root_visual,
                      value_mask, values.data());

    // Create window delete atom used to receive event when window is destroyed
    m_window_delete_atom = Linux::GetInternAtom(m_app_env.connection, "WM_DELETE_WINDOW");
    Linux::SetWindowAtomProperty<xcb_atom_t, 1>(m_app_env.connection, m_dialog_window, "WM_PROTOCOLS", XCB_ATOM_ATOM, {{ m_window_delete_atom }});

    Linux::SetWindowAtomProperty<xcb_atom_t, 4>(m_app_env.connection, m_dialog_window, "WM_STATE", XCB_ATOM_ATOM, {{
        Linux::GetInternAtom(m_app_env.connection, "WM_STATE_SKIP_TASKBAR"),
        Linux::GetInternAtom(m_app_env.connection, "WM_STATE_SKIP_PAGER"),
        Linux::GetInternAtom(m_app_env.connection, "WM_STATE_FOCUSED"),
        Linux::GetInternAtom(m_app_env.connection, "WM_STATE_MODAL")
    }});

    Linux::SetWindowAtomProperty<xcb_window_t, 1>(m_app_env.connection, m_dialog_window, XCB_ATOM_WM_TRANSIENT_FOR, XCB_ATOM_WINDOW, {{ m_app_env.window }});
}

MessageBox::~MessageBox()
{
    META_FUNCTION_TASK();
    xcb_destroy_window(m_app_env.connection, m_dialog_window);
}

void MessageBox::Show(const IApp::Message& message)
{
    m_message = message;

    // Show window on screen
    xcb_map_window(m_app_env.connection, m_dialog_window);
    Linux::SetWindowStringProperty(m_app_env.connection, m_dialog_window, XCB_ATOM_WM_NAME, m_message.title);
    xcb_flush(m_app_env.connection);

    // Event processing loop
    m_is_event_processing = true;
    while (m_is_event_processing)
    {
        while (xcb_generic_event_t* event = xcb_poll_for_event(m_app_env.connection))
        {
            HandleEvent(*event);
            free(event);
        }
    }

    xcb_unmap_window(m_app_env.connection, m_dialog_window);
}

void MessageBox::HandleEvent(xcb_generic_event_t& event)
{
    META_FUNCTION_TASK();
    const uint32_t event_type = event.response_type & 0x7f;
    switch (event_type)
    {
    case XCB_CLIENT_MESSAGE:
        m_is_event_processing = !(m_window_delete_atom && reinterpret_cast<xcb_client_message_event_t&>(event).data.data32[0] == m_window_delete_atom);
        break;

    case XCB_DESTROY_NOTIFY:
        m_is_event_processing = false;
        break;

    case XCB_CONFIGURE_NOTIFY:
        OnWindowResized(reinterpret_cast<const xcb_configure_notify_event_t&>(event));
        break;

    case XCB_KEY_PRESS:
        OnKeyboardChanged(reinterpret_cast<const xcb_key_release_event_t&>(event), true);
        break;

    case XCB_KEY_RELEASE:
        OnKeyboardChanged(reinterpret_cast<const xcb_key_release_event_t&>(event), false);
        break;

    case XCB_BUTTON_PRESS:
        OnMouseButtonChanged(reinterpret_cast<const xcb_button_press_event_t&>(event), true);
        break;

    case XCB_BUTTON_RELEASE:
        OnMouseButtonChanged(reinterpret_cast<const xcb_button_press_event_t&>(event), false);
        break;

    default:
        break;
    }
}

void MessageBox::OnWindowResized(const xcb_configure_notify_event_t& cfg_event)
{
    META_FUNCTION_TASK();
    if (cfg_event.window != m_dialog_window)
        return;
}

void MessageBox::OnKeyboardChanged(const xcb_key_press_event_t& key_press_event, bool is_key_pressed)
{
    META_FUNCTION_TASK();
    META_UNUSED(is_key_pressed);

    XKeyEvent x_key_event{ 0 };
    x_key_event.display = m_app_env.display;
    x_key_event.window = m_dialog_window;
    x_key_event.state = key_press_event.state;
    x_key_event.keycode = key_press_event.detail;

    for (int i = 0; i < 4; ++i)
    {
        const KeySym key_sym = XLookupKeysym(&x_key_event, i);
        META_UNUSED(key_sym);
    }
}

void MessageBox::OnMouseButtonChanged(const xcb_button_press_event_t& button_press_event, bool is_button_pressed)
{
    META_FUNCTION_TASK();
    META_UNUSED(is_button_pressed);
    if (button_press_event.detail != XCB_BUTTON_INDEX_1) // Left mouse button
        return;
}

} // namespace Methane::Platform
