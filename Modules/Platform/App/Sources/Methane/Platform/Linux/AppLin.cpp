/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Platform/Linux/AppLin.cpp
Linux application implementation.

******************************************************************************/

#include <Methane/Platform/Linux/AppLin.h>
#include <Methane/Platform/Utils.h>
#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

#include <string_view>
#include <optional>

namespace Methane::Platform
{

static xcb_intern_atom_reply_t* GetInternAtomReply(xcb_connection_t* connection, std::string_view name) noexcept
{
    META_FUNCTION_TASK();
    const xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, false, name.length(), name.data());
    return xcb_intern_atom_reply(connection, cookie, nullptr);
}

static xcb_atom_t GetInternAtom(xcb_connection_t* xcb_connection, std::string_view name) noexcept
{
    META_FUNCTION_TASK();
    xcb_intern_atom_reply_t* atom_reply = GetInternAtomReply(xcb_connection, name);
    const xcb_atom_t atom = atom_reply ? atom_reply->atom : static_cast<xcb_atom_t>(XCB_ATOM_NONE);
    free(atom_reply);
    return atom;
}

template<typename T>
static std::optional<T> GetWindowPropertyValue(xcb_connection_t* connection, xcb_window_t window, xcb_atom_t atom)
{
    META_FUNCTION_TASK();
    xcb_get_property_cookie_t cookie = xcb_get_property(connection, false, window, atom, XCB_ATOM_ATOM, 0, 32);
    xcb_get_property_reply_t* reply = xcb_get_property_reply(connection, cookie, nullptr);
    const std::optional<T> value_opt = reply ? std::optional<T>(*reinterpret_cast<T*>(xcb_get_property_value(reply))) : std::nullopt;
    free(reply);
    return value_opt;
}

AppLin::AppLin(const AppBase::Settings& settings)
    : AppBase(settings)
{
    META_FUNCTION_TASK();

    // Establish connection to X-server
    int screen_id = 0;
    m_env.connection = xcb_connect(nullptr, &screen_id);
    const int connection_error = xcb_connection_has_error(m_env.connection);
    META_CHECK_ARG_EQUAL_DESCR(connection_error, 0, "XCB connection to display has failed");

    // Find default screen_id setup
    const xcb_setup_t*    setup = xcb_get_setup(m_env.connection);
    xcb_screen_iterator_t screen_iter = xcb_setup_roots_iterator(setup);
    while (screen_id-- > 0)
        xcb_screen_next(&screen_iter);
    const xcb_screen_t* screen = screen_iter.data;

    // Prepare initial window properties
    const uint32_t value_mask = XCB_CW_EVENT_MASK;
    const std::array<uint32_t, 1> values{{
         XCB_EVENT_MASK_STRUCTURE_NOTIFY |
         XCB_EVENT_MASK_PROPERTY_CHANGE |
         XCB_EVENT_MASK_KEY_RELEASE |
         XCB_EVENT_MASK_KEY_PRESS |
         XCB_EVENT_MASK_BUTTON_PRESS |
         XCB_EVENT_MASK_BUTTON_RELEASE |
         XCB_EVENT_MASK_POINTER_MOTION |
         XCB_EVENT_MASK_ENTER_WINDOW |
         XCB_EVENT_MASK_LEAVE_WINDOW
    }};

    // Calculate frame size relative to screen_id size in case of floating point value
    const uint16_t frame_width  = settings.is_full_screen
                                ? screen->width_in_pixels
                                : AppBase::GetScaledSize(settings.size.GetWidth(), screen->width_in_pixels);
    const uint16_t frame_height = settings.is_full_screen
                                ? screen->height_in_pixels
                                : AppBase::GetScaledSize(settings.size.GetHeight(), screen->height_in_pixels);
    const int16_t pos_x = settings.is_full_screen ? 0 : static_cast<int16_t>(screen->width_in_pixels - frame_width) / 2;
    const int16_t pos_y = settings.is_full_screen ? 0 : static_cast<int16_t>(screen->height_in_pixels - frame_height) / 2;

    // Create window and position it in the center of the screen_id
    m_env.window = xcb_generate_id(m_env.connection);
    xcb_create_window(m_env.connection, screen->root_depth,
                      m_env.window, screen->root,
                      pos_x, pos_y, frame_width, frame_height, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      value_mask, values.data());

    // Create window delete atom used to receive event when window is destroyed
    const xcb_atom_t protocols_atom = GetInternAtom(m_env.connection, "WM_PROTOCOLS");
    m_window_delete_atom = GetInternAtom(m_env.connection, "WM_DELETE_WINDOW");
    xcb_change_property(m_env.connection, XCB_PROP_MODE_REPLACE,
                        m_env.window, protocols_atom, 4, 32, 1,
                        &m_window_delete_atom);

    // Display application name in window title, dash tooltip and application menu on GNOME and other desktop environment
    SetWindowTitle(settings.name);
    std::string wm_class;
    wm_class = wm_class.insert(0, settings.name);
    wm_class = wm_class.insert(settings.name.size(), 1, '\0');
    wm_class = wm_class.insert(settings.name.size() + 1, settings.name);
    wm_class = wm_class.insert(wm_class.size(), 1, '\0');
    xcb_change_property(m_env.connection, XCB_PROP_MODE_REPLACE,
                        m_env.window, XCB_ATOM_WM_CLASS,XCB_ATOM_STRING, 8,
                        wm_class.size() + 2, wm_class.c_str());

    m_state_atom            = GetInternAtom(m_env.connection,"_NET_WM_STATE");
    m_state_hidden_atom     = GetInternAtom(m_env.connection,"_NET_WM_STATE_HIDDEN");
    m_state_fullscreen_atom = GetInternAtom(m_env.connection, "_NET_WM_STATE_FULLSCREEN");

    if (settings.is_full_screen)
    {
        // Set window state to full-screen_id
        xcb_change_property(m_env.connection, XCB_PROP_MODE_REPLACE, m_env.window, m_state_atom,
                            XCB_ATOM_ATOM, 32, 1, &m_state_fullscreen_atom);
    }
}

AppLin::~AppLin()
{
    META_FUNCTION_TASK();
    xcb_destroy_window(m_env.connection, m_env.window);
    xcb_disconnect(m_env.connection);
}

int AppLin::Run(const RunArgs& args)
{
    // Skip instrumentation META_FUNCTION_TASK() since this is the only root function running till application close
    if (const int base_return_code = AppBase::Run(args);
        base_return_code)
        return base_return_code;

    // Request window geometry
    const xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(m_env.connection, m_env.window);

    // Show window on screen
    xcb_map_window(m_env.connection, m_env.window);
    xcb_flush(m_env.connection);

    // If there's a deferred message, schedule it to show for the current window event loop
    if (HasDeferredMessage())
    {
        ScheduleAlert();
    }

    Data::FrameSize frame_size;
    if (xcb_get_geometry_reply_t* geometry_reply = xcb_get_geometry_reply(m_env.connection, geometry_cookie, nullptr);
        geometry_reply)
    {
        frame_size.SetWidth(geometry_reply->width);
        frame_size.SetHeight(geometry_reply->height);
        free(geometry_reply);
    }

    // Application Initialization
    bool init_success = InitContextWithErrorHandling(m_env, frame_size);
    if (init_success)
    {
        init_success = InitWithErrorHandling();
    }

    // Event processing loop
    m_is_event_processing = true;
    while (m_is_event_processing)
    {
        while (xcb_generic_event_t* event = xcb_poll_for_event(m_env.connection))
        {
            HandleEvent(*event);
            free(event);
        }

        if (!init_success || !m_is_event_processing)
            continue;

        if (IsResizing())
            EndResizing();

        UpdateAndRenderWithErrorHandling();
    }

    return 0;
}

void AppLin::Alert(const Message& msg, bool deferred)
{
    META_FUNCTION_TASK();
    AppBase::Alert(msg, deferred);
}

void AppLin::SetWindowTitle(const std::string& title_text)
{
    META_FUNCTION_TASK();
    xcb_change_property(m_env.connection, XCB_PROP_MODE_REPLACE,
                        m_env.window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
                        title_text.length() + 1, title_text.c_str());
}

bool AppLin::SetFullScreen(bool is_full_screen)
{
    META_FUNCTION_TASK();
    if (!AppBase::SetFullScreen(is_full_screen))
        return false;

    return true;
}

void AppLin::Close()
{
    META_FUNCTION_TASK();
}

void AppLin::ShowAlert(const Message& /*msg*/)
{
    META_FUNCTION_TASK();
}

void AppLin::ScheduleAlert()
{
    META_FUNCTION_TASK();
}

void AppLin::HandleEvent(xcb_generic_event_t& event)
{
    META_FUNCTION_TASK();
    const uint32_t xcb_event_type = event.response_type & 0x7f;
    switch (xcb_event_type)
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

    case XCB_PROPERTY_NOTIFY:
        OnPropertyChanged(reinterpret_cast<const xcb_property_notify_event_t&>(event));
        break;

    case XCB_KEY_PRESS:
        OnKeyboardChanged(reinterpret_cast<const xcb_key_release_event_t&>(event), Keyboard::KeyState::Pressed);
        break;

    case XCB_KEY_RELEASE:
        OnKeyboardChanged(reinterpret_cast<const xcb_key_release_event_t&>(event), Keyboard::KeyState::Released);
        break;

    case XCB_BUTTON_PRESS:
        OnMouseButtonChanged(reinterpret_cast<const xcb_button_press_event_t&>(event), Mouse::ButtonState::Pressed);
        break;

    case XCB_BUTTON_RELEASE:
        OnMouseButtonChanged(reinterpret_cast<const xcb_button_press_event_t&>(event), Mouse::ButtonState::Released);
        break;

    case XCB_MOTION_NOTIFY:
        OnMouseMoved(reinterpret_cast<const xcb_motion_notify_event_t&>(event));
        break;

    case XCB_ENTER_NOTIFY:
        OnMouseInWindowChanged(reinterpret_cast<const xcb_enter_notify_event_t&>(event), true);
        break;

    case XCB_LEAVE_NOTIFY:
        OnMouseInWindowChanged(reinterpret_cast<const xcb_enter_notify_event_t&>(event), false);
        break;

    default:
        break;
    }
}

void AppLin::OnWindowResized(const xcb_configure_notify_event_t& cfg_event)
{
    META_FUNCTION_TASK();
    if (!IsResizing())
        StartResizing();

    if (cfg_event.width == 0 || cfg_event.height == 0 ||
        !Resize(Data::FrameSize(cfg_event.width, cfg_event.height), false))
        return;

    if (IsResizing())
    {
        UpdateAndRenderWithErrorHandling();
    }
}

void AppLin::OnPropertyChanged(const xcb_property_notify_event_t& prop_event)
{
    META_FUNCTION_TASK();
    if (prop_event.atom != m_state_atom)
        return;

    const std::optional<xcb_atom_t> state_value_opt = GetWindowPropertyValue<xcb_atom_t>(m_env.connection, m_env.window, m_state_atom);
    if (!state_value_opt)
        return;

    if (state_value_opt == m_state_hidden_atom)
    {
        // Window was minimized
        Resize(GetFrameSize(), true);
    }
    else if (IsMinimized())
    {
        // Window was shown
        Resize(GetFrameSize(), false);
    }
}

void AppLin::OnKeyboardChanged(const xcb_key_press_event_t& key_press_event, Keyboard::KeyState key_state)
{
    META_FUNCTION_TASK();
    const Keyboard::Key key = Keyboard::KeyConverter({ key_press_event.detail, key_press_event.state }).GetKey();
    if (key == Keyboard::Key::Unknown)
        return;

    ProcessInputWithErrorHandling(&Input::IActionController::OnKeyboardChanged, key, key_state);
}

void AppLin::OnMouseButtonChanged(const xcb_button_press_event_t& button_press_event, Mouse::ButtonState button_state)
{
    META_FUNCTION_TASK();
    Mouse::Button button = Mouse::Button::Unknown;
    int delta_sign = -1;

    switch(button_press_event.detail)
    {
    case XCB_BUTTON_INDEX_1: button = Mouse::Button::Left; break;
    case XCB_BUTTON_INDEX_2: button = Mouse::Button::Middle; break;
    case XCB_BUTTON_INDEX_3: button = Mouse::Button::Right; break;
    case XCB_BUTTON_INDEX_4: delta_sign = 1; [[fallthrough]];
    case XCB_BUTTON_INDEX_5: button = Mouse::Button::VScroll; break;
    case XCB_BUTTON_INDEX_5 + 1: delta_sign = 1; [[fallthrough]];
    case XCB_BUTTON_INDEX_5 + 2: button = Mouse::Button::HScroll; break;
    default: META_UNEXPECTED_ARG_DESCR(button_press_event.detail, "Mouse button is not supported");
    }

    ProcessInputWithErrorHandling(&Input::IActionController::OnMouseButtonChanged, button, button_state);

    if ((button == Mouse::Button::HScroll || button == Mouse::Button::VScroll) && button_state == Mouse::ButtonState::Released)
    {
        const float scroll_value = button_press_event.state
                                 ? static_cast<float>(delta_sign * button_press_event.state / 1024)
                                 : static_cast<float>(delta_sign);
        const Mouse::Scroll mouse_scroll(
            button == Mouse::Button::HScroll ? scroll_value : 0.F,
            button == Mouse::Button::VScroll ? scroll_value : 0.F
        );
        ProcessInputWithErrorHandling(&Input::IActionController::OnMouseScrollChanged, mouse_scroll);
    }
}

void AppLin::OnMouseMoved(const xcb_motion_notify_event_t& motion_event)
{
    META_FUNCTION_TASK();
    const Mouse::Position mouse_pos(motion_event.event_x, motion_event.event_y);
    ProcessInputWithErrorHandling(&Input::IActionController::OnMousePositionChanged, mouse_pos);
}

void AppLin::OnMouseInWindowChanged(const xcb_enter_notify_event_t& enter_event, bool mouse_in_window)
{
    META_FUNCTION_TASK();
    META_UNUSED(enter_event);
    ProcessInputWithErrorHandling(&Input::IActionController::OnMouseInWindowChanged, mouse_in_window);
}

} // namespace Methane::Platform