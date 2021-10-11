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
#include <Methane/Platform/Utils.h>
#include <Methane/Instrumentation.h>

#include <string_view>
#include <optional>

#include <X11/Xutil.h>

namespace Methane::Platform
{

static const std::string_view g_default_font_name = "-*-*-medium-r-normal--0-120-*-*-p-0-iso8859-1";

MessageBox::MessageBox(const AppEnvironment& app_env)
    : m_app_env(app_env)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(m_app_env.display, "X11 display should be opened");
    META_CHECK_ARG_NOT_NULL_DESCR(m_app_env.screen, "XCB screen should be initialized");
    META_CHECK_ARG_NOT_NULL_DESCR(m_app_env.connection, "XCB connection should be initialized");

    // Prepare initial window properties
    const uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    const std::array<uint32_t, 2> values{{
        m_app_env.screen->white_pixel,
        XCB_EVENT_MASK_EXPOSURE |
        XCB_EVENT_MASK_STRUCTURE_NOTIFY |
        XCB_EVENT_MASK_KEY_RELEASE |
        XCB_EVENT_MASK_KEY_PRESS |
        XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE
    }};

    // Calculate frame size relative to screen_id size in case of floating point value
    const uint16_t frame_width  = 640;
    const uint16_t frame_height = 240;

    // Create window and position it in the center of the screen_id
    m_dialog_window = xcb_generate_id(m_app_env.connection);
    Linux::XcbCheck(xcb_create_window_checked(m_app_env.connection, m_app_env.screen->root_depth,
                                              m_dialog_window, m_app_env.screen->root,
                                              0, 0, frame_width, frame_height, 1,
                                              XCB_WINDOW_CLASS_INPUT_OUTPUT, m_app_env.screen->root_visual,
                                              value_mask, values.data()),
                    m_app_env.connection, "failed to create message box window");

    // Create window delete atom used to receive event when window is destroyed
    m_window_delete_atom = Linux::GetXcbInternAtom(m_app_env.connection, "WM_DELETE_WINDOW");
    Linux::SetXcbWindowAtomProperty<xcb_atom_t, 1>(m_app_env.connection, m_dialog_window, "WM_PROTOCOLS", XCB_ATOM_ATOM, { { m_window_delete_atom } });

    Linux::SetXcbWindowAtomProperty<xcb_atom_t, 4>(m_app_env.connection, m_dialog_window, "WM_STATE", XCB_ATOM_ATOM, { {
        Linux::GetXcbInternAtom(m_app_env.connection, "WM_STATE_SKIP_TASKBAR"),
        Linux::GetXcbInternAtom(m_app_env.connection, "WM_STATE_SKIP_PAGER"),
        Linux::GetXcbInternAtom(m_app_env.connection, "WM_STATE_FOCUSED"),
        Linux::GetXcbInternAtom(m_app_env.connection, "WM_STATE_MODAL")
    } });

    Linux::SetXcbWindowAtomProperty<xcb_atom_t, 1>(m_app_env.connection, m_dialog_window, "WM_WINDOW_TYPE", XCB_ATOM_ATOM, { {
        Linux::GetXcbInternAtom(m_app_env.connection, "WM_WINDOW_TYPE_DIALOG")
    } });

    Linux::SetXcbWindowAtomProperty<xcb_window_t, 1>(m_app_env.connection, m_dialog_window, "WM_TRANSIENT_FOR", XCB_ATOM_WINDOW, { { m_app_env.window } });

    // Create font
    m_font = xcb_generate_id(m_app_env.connection);
    Linux::XcbCheck(xcb_open_font_checked(m_app_env.connection, m_font, g_default_font_name.length(), g_default_font_name.data()),
                    m_app_env.connection, "failed to open font");

    // Create graphics context
    m_gfx_context = xcb_generate_id(m_app_env.connection);
    const uint32_t gfx_context_values_mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT | XCB_GC_GRAPHICS_EXPOSURES;
    const std::array<uint32_t, 4> gfx_context_values{{
        m_app_env.screen->black_pixel,
        m_app_env.screen->white_pixel,
        m_font,
        0
    }};
    Linux::XcbCheck(xcb_create_gc_checked(m_app_env.connection, m_gfx_context, m_app_env.screen->root, gfx_context_values_mask, gfx_context_values.data()),
                    m_app_env.connection, "failed to create font context");
}

MessageBox::~MessageBox()
{
    META_FUNCTION_TASK();
    Linux::XcbCheck(xcb_close_font_checked(m_app_env.connection, m_font), m_app_env.connection, "failed to close font");
    xcb_free_gc(m_app_env.connection, m_gfx_context);
    xcb_destroy_window(m_app_env.connection, m_dialog_window);
}

void MessageBox::Show(const IApp::Message& message)
{
    m_message = message;

    // Show window on screen
    xcb_map_window(m_app_env.connection, m_dialog_window);
    Linux::SetXcbWindowStringProperty(m_app_env.connection, m_dialog_window, XCB_ATOM_WM_NAME, m_message.title);
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

void MessageBox::HandleEvent(const xcb_generic_event_t& event)
{
    META_FUNCTION_TASK();
    const uint32_t event_type = event.response_type & 0x7f;
    switch (event_type)
    {
    case XCB_CLIENT_MESSAGE:
        m_is_event_processing = !(m_window_delete_atom && reinterpret_cast<const xcb_client_message_event_t&>(event).data.data32[0] == m_window_delete_atom);
        break;

    case XCB_DESTROY_NOTIFY:
        m_is_event_processing = false;
        break;

    case XCB_EXPOSE:
        Draw(reinterpret_cast<const xcb_expose_event_t&>(event));
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

void MessageBox::Draw(const xcb_expose_event_t&)
{
    META_FUNCTION_TASK();

    const int16_t margin_size = 10;
    int16_t x_pos = margin_size;
    int16_t y_pos = margin_size;
    uint32_t text_width = 0U;
    uint32_t text_height = 0U;
    uint32_t line_height = 0U;
    const std::vector<std::string_view> info_lines = SplitString(m_message.information, '\n', true);

    for(const std::string_view& info_line : info_lines)
    {
        if (info_line.empty())
        {
            y_pos += line_height;
            text_height += line_height;
            continue;
        }

        uint32_t line_width  = 0U;
        uint32_t line_ascent = 0U;
        Linux::XcbMeasureText(m_app_env.connection, m_font, info_line, line_width, line_height, line_ascent);
        Linux::XcbCheck(xcb_image_text_8_checked(m_app_env.connection, info_line.length(), m_dialog_window, m_gfx_context, x_pos, y_pos + line_ascent, info_line.data()),
                        m_app_env.connection, "failed to draw text");
        y_pos += line_height;
        text_height += line_height;
        text_width = std::max(text_width, line_width);
    }

    Resize(text_width + margin_size * 2, text_height + margin_size * 2);

    xcb_flush(m_app_env.connection);
}

void MessageBox::Resize(int width, int height)
{
    META_FUNCTION_TASK();

    struct WMSizeHints
    {
        uint32_t flags;
        int32_t  x, y;
        int32_t  width, height;
        int32_t  min_width, min_height;
        int32_t  max_width, max_height;
        int32_t  width_inc, height_inc;
        int32_t  min_aspect_num, min_aspect_den;
        int32_t  max_aspect_num, max_aspect_den;
        int32_t  base_width, base_height;
        uint32_t win_gravity;
    };

    WMSizeHints size_hints{0};
    size_hints.flags       = PWinGravity | PSize | PMinSize | PMaxSize;
    size_hints.win_gravity = XCB_GRAVITY_CENTER;
    size_hints.width = width;
    size_hints.height = height;
    size_hints.min_width = width;
    size_hints.min_height = height;
    size_hints.max_width = width;
    size_hints.max_height = height;

    Linux::SetXcbWindowAtomProperty<WMSizeHints, 1>(m_app_env.connection, m_dialog_window, XCB_ATOM_WM_NORMAL_HINTS, XCB_ATOM_WM_SIZE_HINTS, {{ size_hints }});
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
