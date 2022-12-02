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
#include <Methane/Platform/Input/Keyboard.h>
#include <Methane/Platform/Utils.h>
#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

#include <string_view>
#include <optional>

#include <X11/Xutil.h>

namespace Methane::Platform
{

static const std::string_view g_default_font_name = "-*-fixed-medium-r-*--15-*-*-*-*-*-*-*";
static const int16_t g_margin_size  = 30;
static const int16_t g_padding_size = 15;
static const size_t g_max_line_length = 100U;

struct MessageButtonStyle
{
    std::string_view label;
    Linux::SystemColor default_back_color = Linux::SystemColor::ButtonBackgroundNormal;
    Linux::SystemColor hovered_back_color = Linux::SystemColor::ButtonBackgroundHovered;
};

MessageButtonStyle GetMessageButtonStyle(IApp::Message::Type message_type)
{
    META_FUNCTION_TASK();
    switch(message_type)
    {
    case IApp::Message::Type::Information: return { "OK",       Linux::SystemColor::ButtonBackgroundNormal,  Linux::SystemColor::ButtonBackgroundHovered };
    case IApp::Message::Type::Warning:     return { "Continue", Linux::SystemColor::ButtonBackgroundWarning, Linux::SystemColor::ButtonBackgroundWarningHovered };
    case IApp::Message::Type::Error:       return { "Close",    Linux::SystemColor::ButtonBackgroundError,   Linux::SystemColor::ButtonBackgroundErrorHovered };
    default:
        META_UNEXPECTED_ARG_RETURN(message_type, MessageButtonStyle{}); 
    }
}

MessageBox::MessageBox(const AppEnvironment& app_env)
    : m_app_env(app_env)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(m_app_env.display, "X11 display should be opened");
    META_CHECK_ARG_NOT_NULL_DESCR(m_app_env.screen, "XCB screen should be initialized");
    META_CHECK_ARG_NOT_NULL_DESCR(m_app_env.connection, "XCB connection should be initialized");

    // Prepare initial window properties
    const uint32_t back_color = Linux::GetXcbSystemColor(Linux::SystemColor::Background);
    const uint32_t text_color = Linux::GetXcbSystemColor(Linux::SystemColor::DefaultText);
    const uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    const std::array<uint32_t, 2> values{{
        back_color,
        XCB_EVENT_MASK_EXPOSURE |
        XCB_EVENT_MASK_STRUCTURE_NOTIFY |
        XCB_EVENT_MASK_KEY_RELEASE |
        XCB_EVENT_MASK_KEY_PRESS |
        XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_POINTER_MOTION
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
    Linux::SetXcbWindowAtomProperty<xcb_atom_t>(m_app_env.connection, m_dialog_window, "WM_PROTOCOLS", XCB_ATOM_ATOM, { m_window_delete_atom });

    Linux::SetXcbWindowAtomProperty<xcb_atom_t>(m_app_env.connection, m_dialog_window, "WM_STATE", XCB_ATOM_ATOM, {
        Linux::GetXcbInternAtom(m_app_env.connection, "WM_STATE_SKIP_TASKBAR"),
        Linux::GetXcbInternAtom(m_app_env.connection, "WM_STATE_SKIP_PAGER"),
        Linux::GetXcbInternAtom(m_app_env.connection, "WM_STATE_FOCUSED"),
        Linux::GetXcbInternAtom(m_app_env.connection, "WM_STATE_MODAL")
    });

    Linux::SetXcbWindowAtomProperty<xcb_atom_t>(m_app_env.connection, m_dialog_window, "WM_WINDOW_TYPE", XCB_ATOM_ATOM, {
        Linux::GetXcbInternAtom(m_app_env.connection, "WM_WINDOW_TYPE_DIALOG")
    });

    Linux::SetXcbWindowAtomProperty<xcb_window_t>(m_app_env.connection, m_dialog_window, "WM_TRANSIENT_FOR", XCB_ATOM_WINDOW, { m_app_env.window });

    // Create default font
    m_default_font = xcb_generate_id(m_app_env.connection);
    Linux::XcbCheck(xcb_open_font_checked(m_app_env.connection, m_default_font, g_default_font_name.length(), g_default_font_name.data()),
                    m_app_env.connection, "failed to open default font");

    // Create graphics context
    m_gfx_context = xcb_generate_id(m_app_env.connection);
    const uint32_t gfx_context_values_mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_LINE_WIDTH | XCB_GC_FONT | XCB_GC_GRAPHICS_EXPOSURES;
    const std::array<uint32_t, 5> gfx_context_values{{ text_color, back_color, 2, m_default_font, 0 }};
    Linux::XcbCheck(xcb_create_gc_checked(m_app_env.connection, m_gfx_context, m_app_env.screen->root, gfx_context_values_mask, gfx_context_values.data()),
                    m_app_env.connection, "failed to create font context");
}

MessageBox::~MessageBox()
{
    META_FUNCTION_TASK();
    Linux::XcbCheck(xcb_close_font_checked(m_app_env.connection, m_default_font), m_app_env.connection, "failed to close font");
    xcb_free_gc(m_app_env.connection, m_gfx_context);
    xcb_destroy_window(m_app_env.connection, m_dialog_window);
}

void MessageBox::Show(const IApp::Message& message)
{
    m_message = message;

    // Update window title
    Linux::SetXcbWindowStringProperty(m_app_env.connection, m_dialog_window, XCB_ATOM_WM_NAME, m_message.title);

    // Show window on screen
    xcb_map_window(m_app_env.connection, m_dialog_window);
    xcb_flush(m_app_env.connection);

    // Event processing loop
    m_is_event_processing = true;
    while (m_is_event_processing)
    {
        while (xcb_generic_event_t* event = xcb_poll_for_event(m_app_env.connection))
        {
            HandleEvent(*event);
            free(event); // NOSONAR
        }
    }

    // Hide window
    xcb_unmap_window(m_app_env.connection, m_dialog_window);
    xcb_flush(m_app_env.connection);
}

void MessageBox::HandleEvent(const xcb_generic_event_t& event)
{
    META_FUNCTION_TASK();
    const uint8_t event_type = event.response_type & 0x7f; // NOSONAR
    switch (event_type)
    {
    case XCB_CLIENT_MESSAGE:
        m_is_event_processing = !(m_window_delete_atom && reinterpret_cast<const xcb_client_message_event_t&>(event).data.data32[0] == m_window_delete_atom); // NOSONAR
        break;

    case XCB_DESTROY_NOTIFY:
        m_is_event_processing = false;
        break;

    case XCB_EXPOSE:
        DrawDialog();
        break;

    case XCB_KEY_PRESS:
        OnKeyboardChanged(reinterpret_cast<const xcb_key_release_event_t&>(event), true); // NOSONAR
        break;

    case XCB_KEY_RELEASE:
        OnKeyboardChanged(reinterpret_cast<const xcb_key_release_event_t&>(event), false); // NOSONAR
        break;

    case XCB_BUTTON_PRESS:
        OnMouseButtonChanged(reinterpret_cast<const xcb_button_press_event_t&>(event), true); // NOSONAR
        break;

    case XCB_BUTTON_RELEASE:
        OnMouseButtonChanged(reinterpret_cast<const xcb_button_press_event_t&>(event), false); // NOSONAR
        break;

    case XCB_MOTION_NOTIFY:
        OnMouseMoved(reinterpret_cast<const xcb_motion_notify_event_t&>(event)); // NOSONAR
        break;

    default:
        break;
    }
}

void MessageBox::DrawDialog()
{
    META_FUNCTION_TASK();

    int16_t x_pos = g_margin_size;
    int16_t y_pos = g_margin_size;
    uint32_t text_width = 600U; // minimum reserved width
    uint32_t text_height = 0U;
    uint32_t line_height = 0U;

    // Draw information text
    const uint32_t info_text_mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND;
    const std::array<uint32_t, 3> info_text_values{{
        Linux::GetXcbSystemColor(Linux::SystemColor::DefaultText),
        Linux::GetXcbSystemColor(Linux::SystemColor::Background)
    }};
    Linux::XcbCheck(xcb_change_gc_checked(m_app_env.connection, m_gfx_context, info_text_mask, info_text_values.data()),
                    m_app_env.connection, "failed to change graphics context parameters");

    const std::vector<std::string_view> info_lines = SplitString(m_message.information, '\n', true, g_max_line_length);
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
        Linux::XcbMeasureText(m_app_env.connection, m_default_font, info_line, line_width, line_height, line_ascent);
        Linux::XcbCheck(xcb_image_text_8_checked(m_app_env.connection, static_cast<uint8_t>(info_line.length()), m_dialog_window, m_gfx_context,
                                                 x_pos, static_cast<int16_t>(y_pos + line_ascent), info_line.data()),
                        m_app_env.connection, "failed to draw message box information text");
        y_pos += line_height;
        text_height += line_height;
        text_width = std::max(text_width, line_width);
    }

    const uint32_t button_height = line_height + g_padding_size * 2;
    m_dialog_size = { text_width + g_margin_size * 2, text_height + button_height + g_margin_size * 3 };

    Resize(static_cast<int>(m_dialog_size.GetWidth()), static_cast<int>(m_dialog_size.GetHeight()));

    DrawButtons();
}

void MessageBox::DrawButtons()
{
    META_FUNCTION_TASK();
    const MessageButtonStyle button_style = GetMessageButtonStyle(m_message.type);
    const Linux::SystemColor button_pressed_color_type    = m_mouse_pressed_ok_button ? Linux::SystemColor::ButtonBackgroundPressed : button_style.hovered_back_color;
    const Linux::SystemColor button_background_color_type = m_mouse_over_ok_button ? button_pressed_color_type : button_style.default_back_color;

    // Measure text of button label
    uint32_t ok_label_width  = 0U;
    uint32_t ok_label_height = 0U;
    uint32_t ok_label_ascent = 0U;
    Linux::XcbMeasureText(m_app_env.connection, m_default_font, button_style.label, ok_label_width, ok_label_height, ok_label_ascent);

    // Calculate button rectangle
    const uint32_t button_height = ok_label_height + g_padding_size * 2;
    const uint32_t button_width  = button_height * 4;
    m_ok_button_rect = {
        static_cast<int16_t>((m_dialog_size.GetWidth() - button_width) / 2),
        static_cast<int16_t>(m_dialog_size.GetHeight() - button_height - g_margin_size),
        static_cast<uint16_t>(button_width),
        static_cast<uint16_t>(button_height)
    };

    // Draw button background
    const uint32_t button_back_mask = XCB_GC_FOREGROUND;
    const std::array<uint32_t, 1> button_back_values{{ Linux::GetXcbSystemColor(button_background_color_type) }};
    Linux::XcbCheck(xcb_change_gc_checked(m_app_env.connection, m_gfx_context, button_back_mask, button_back_values.data()),
                    m_app_env.connection, "failed to change graphics context parameters");
    Linux::XcbCheck(xcb_poly_fill_rectangle_checked(m_app_env.connection, m_dialog_window, m_gfx_context, 1, &m_ok_button_rect),
                    m_app_env.connection, "failed to draw OK button rectangle");

    // Draw button border
    const uint32_t button_border_mask = XCB_GC_FOREGROUND | XCB_GC_LINE_WIDTH;
    const std::array<uint32_t, 2> button_border_values{{ Linux::GetXcbSystemColor(Linux::SystemColor::ButtonBorderSelected), 2 }};
    Linux::XcbCheck(xcb_change_gc_checked(m_app_env.connection, m_gfx_context, button_border_mask, button_border_values.data()),
                    m_app_env.connection, "failed to change graphics context parameters");
    Linux::XcbCheck(xcb_poly_rectangle_checked(m_app_env.connection, m_dialog_window, m_gfx_context, 1, &m_ok_button_rect),
                    m_app_env.connection, "failed to draw OK button rectangle");

    // Draw button label text
    const int16_t ok_label_x = m_ok_button_rect.x + static_cast<int16_t>((m_ok_button_rect.width - ok_label_width) / 2);
    const int16_t ok_label_y = m_ok_button_rect.y + static_cast<int16_t>((m_ok_button_rect.height - ok_label_height) / 2);
    const uint32_t button_label_mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT;
    const std::array<uint32_t, 3> button_label_values{{
        Linux::GetXcbSystemColor(Linux::SystemColor::DefaultText),
        Linux::GetXcbSystemColor(button_background_color_type),
        m_default_font
    }};
    Linux::XcbCheck(xcb_change_gc_checked(m_app_env.connection, m_gfx_context, button_label_mask, button_label_values.data()),
                    m_app_env.connection, "failed to change graphics context parameters");
    Linux::XcbCheck(xcb_image_text_8_checked(m_app_env.connection, static_cast<uint8_t>(button_style.label.length()), m_dialog_window, m_gfx_context,
                                             ok_label_x, static_cast<int16_t>(ok_label_y + ok_label_ascent), button_style.label.data()),
                    m_app_env.connection, "failed to draw button label text");

    xcb_flush(m_app_env.connection);
}

void MessageBox::Resize(int width, int height) const
{
    META_FUNCTION_TASK();
    Linux::WMSizeHints size_hints{0};
    size_hints.flags       = PWinGravity | PSize | PMinSize | PMaxSize;
    size_hints.win_gravity = XCB_GRAVITY_CENTER;
    size_hints.width = width;
    size_hints.height = height;
    size_hints.min_width = width;
    size_hints.min_height = height;
    size_hints.max_width = width;
    size_hints.max_height = height;

    Linux::SetXcbWindowAtomProperty<Linux::WMSizeHints>(m_app_env.connection, m_dialog_window, XCB_ATOM_WM_NORMAL_HINTS, XCB_ATOM_WM_SIZE_HINTS, { size_hints });
}

void MessageBox::OnKeyboardChanged(const xcb_key_press_event_t& key_press_event, bool is_key_pressed)
{
    META_FUNCTION_TASK();
    META_UNUSED(is_key_pressed);
    const Input::Keyboard::Key key = Linux::ConvertXcbKey(m_app_env.display, m_app_env.window, key_press_event.detail, key_press_event.state);

    // Close message box when Enter or Escape key is released
    if (!is_key_pressed && (key == Input::Keyboard::Key::Enter || key == Input::Keyboard::Key::KeyPadEnter || key == Input::Keyboard::Key::Escape))
        m_is_event_processing = false;
}

void MessageBox::OnMouseMoved(const xcb_motion_notify_event_t& motion_event)
{
    META_FUNCTION_TASK();
    const bool mouse_was_over_ok_button = m_mouse_over_ok_button;

    m_mouse_state.SetPosition(Mouse::Position(motion_event.event_x, motion_event.event_y));
    m_mouse_over_ok_button = m_ok_button_rect.x <= motion_event.event_x && motion_event.event_x <= m_ok_button_rect.x + static_cast<int16_t>(m_ok_button_rect.width) &&
                             m_ok_button_rect.y <= motion_event.event_y && motion_event.event_y <= m_ok_button_rect.y + static_cast<int16_t>(m_ok_button_rect.height);

    if (m_mouse_over_ok_button != mouse_was_over_ok_button)
        DrawButtons();
}

void MessageBox::OnMouseButtonChanged(const xcb_button_press_event_t& button_press_event, bool is_button_pressed)
{
    META_FUNCTION_TASK();
    const bool mouse_was_pressing_ok_button = m_mouse_pressed_ok_button;

    m_mouse_state.SetButton(Linux::ConvertXcbMouseButton(button_press_event.detail).first,
                            is_button_pressed ? Input::Mouse::ButtonState::Pressed : Input::Mouse::ButtonState::Released);
    m_mouse_pressed_ok_button = m_mouse_over_ok_button && m_mouse_state.GetPressedButtons().count(Mouse::Button::Left);

    if (m_mouse_pressed_ok_button != mouse_was_pressing_ok_button)
    {
        DrawButtons();

        // Close the dialog when mouse button is released over OK button
        if (!m_mouse_pressed_ok_button && m_mouse_over_ok_button)
            m_is_event_processing = false;
    }
}

} // namespace Methane::Platform
