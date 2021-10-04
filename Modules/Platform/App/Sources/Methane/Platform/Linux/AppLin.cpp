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

static xcb_intern_atom_reply_t* GetInternAtomReply(xcb_connection_t* xcb_connection, std::string_view name) noexcept
{
    META_FUNCTION_TASK();
    const xcb_intern_atom_cookie_t xcb_cookie = xcb_intern_atom(xcb_connection, false, name.length(), name.data());
    return xcb_intern_atom_reply(xcb_connection, xcb_cookie, nullptr);
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
    int screen = 0;
    m_env.connection = xcb_connect(nullptr, &screen);
    const int xcb_connection_error = xcb_connection_has_error(m_env.connection);
    META_CHECK_ARG_EQUAL_DESCR(xcb_connection_error, 0, "XCB connection to display has failed");

    // Find default screen setup
    const xcb_setup_t*    xcb_setup       = xcb_get_setup(m_env.connection);
    xcb_screen_iterator_t xcb_screen_iter = xcb_setup_roots_iterator(xcb_setup);
    while (screen-- > 0)
        xcb_screen_next(&xcb_screen_iter);
    const xcb_screen_t* xcb_screen = xcb_screen_iter.data;

    // Prepare initial window properties
    const uint32_t value_mask = XCB_CW_EVENT_MASK;
    const std::array<uint32_t, 1> values{{
         XCB_EVENT_MASK_KEY_RELEASE |
         XCB_EVENT_MASK_KEY_PRESS |
         XCB_EVENT_MASK_EXPOSURE |
         XCB_EVENT_MASK_STRUCTURE_NOTIFY |
         XCB_EVENT_MASK_PROPERTY_CHANGE |
         XCB_EVENT_MASK_POINTER_MOTION |
         XCB_EVENT_MASK_BUTTON_PRESS |
         XCB_EVENT_MASK_BUTTON_RELEASE
    }};

    // Calculate frame size relative to screen size in case of floating point value
    const uint16_t frame_width  = settings.is_full_screen
                                ? xcb_screen->width_in_pixels
                                : AppBase::GetScaledSize(settings.size.GetWidth(), xcb_screen->width_in_pixels);
    const uint16_t frame_height = settings.is_full_screen
                                ? xcb_screen->height_in_pixels
                                : AppBase::GetScaledSize(settings.size.GetHeight(), xcb_screen->height_in_pixels);
    const int16_t pos_x = settings.is_full_screen ? 0 : static_cast<int16_t>(xcb_screen->width_in_pixels - frame_width) / 2;
    const int16_t pos_y = settings.is_full_screen ? 0 : static_cast<int16_t>(xcb_screen->height_in_pixels - frame_height) / 2;

    // Create window and position it in the center of the screen
    m_env.window = xcb_generate_id(m_env.connection);
    xcb_create_window(m_env.connection, xcb_screen->root_depth,
                      m_env.window, xcb_screen->root,
                      pos_x, pos_y, frame_width, frame_height, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, xcb_screen->root_visual,
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
        // Set window state to full-screen
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
    const xcb_get_geometry_cookie_t xcb_geometry_cookie = xcb_get_geometry(m_env.connection, m_env.window);

    // Show window on screen
    xcb_map_window(m_env.connection, m_env.window);
    xcb_flush(m_env.connection);

    // If there's a deferred message, schedule it to show for the current window event loop
    if (HasDeferredMessage())
    {
        ScheduleAlert();
    }

    Data::FrameSize frame_size;
    if (xcb_get_geometry_reply_t* xcb_geometry_reply = xcb_get_geometry_reply(m_env.connection, xcb_geometry_cookie, nullptr);
        xcb_geometry_reply)
    {
        frame_size.SetWidth(xcb_geometry_reply->width);
        frame_size.SetHeight(xcb_geometry_reply->height);
        free(xcb_geometry_reply);
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
        while (xcb_generic_event_t* xcb_event = xcb_poll_for_event(m_env.connection))
        {
            HandleEvent(*xcb_event);
            free(xcb_event);
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

void AppLin::HandleEvent(xcb_generic_event_t& xcb_event)
{
    META_FUNCTION_TASK();
    const uint32_t xcb_event_type = xcb_event.response_type & 0x7f;
    switch (xcb_event_type)
    {
    case XCB_CLIENT_MESSAGE:
        m_is_event_processing = !(m_window_delete_atom && reinterpret_cast<xcb_client_message_event_t&>(xcb_event).data.data32[0] == m_window_delete_atom);
        break;

    case XCB_DESTROY_NOTIFY:
        m_is_event_processing = false;
        break;

    case XCB_CONFIGURE_NOTIFY:
        OnWindowResized(reinterpret_cast<const xcb_configure_notify_event_t&>(xcb_event));
        break;

    case XCB_PROPERTY_NOTIFY:
        OnPropertyChanged(reinterpret_cast<const xcb_property_notify_event_t&>(xcb_event));
        break;

    default:
        break;
    }
}

void AppLin::OnWindowResized(const xcb_configure_notify_event_t& xcb_cfg_event)
{
    META_FUNCTION_TASK();
    if (!IsResizing())
        StartResizing();


    if (xcb_cfg_event.width == 0 || xcb_cfg_event.height == 0 ||
        !Resize(Data::FrameSize(xcb_cfg_event.width, xcb_cfg_event.height), false))
        return;

    if (IsResizing())
    {
        UpdateAndRenderWithErrorHandling();
    }
}

void AppLin::OnPropertyChanged(const xcb_property_notify_event_t& xcb_prop_event)
{
    META_FUNCTION_TASK();
    if (xcb_prop_event.atom != m_state_atom)
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

} // namespace Methane::Platform