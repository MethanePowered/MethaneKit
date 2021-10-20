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

#include "MessageBox.h"
#include "XcbUtils.h"

#include <Methane/Platform/Linux/AppLin.h>
#include <Methane/Platform/Utils.h>
#include <Methane/Data/Provider.h>
#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

#include <string_view>
#include <optional>

#include <stb_image.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xutil.h>

namespace Methane::Platform
{

AppLin::AppLin(const AppBase::Settings& settings)
    : AppBase(settings)
{
    META_FUNCTION_TASK();

    m_env.display = XOpenDisplay(nullptr);
    META_CHECK_ARG_NOT_NULL_DESCR(m_env.display, "failed to open X11 display");
    XSetEventQueueOwner(m_env.display, XCBOwnsEventQueue);

    // Establish connection to X-server
    m_env.connection = XGetXCBConnection(m_env.display);
    const int connection_error = xcb_connection_has_error(m_env.connection);
    META_CHECK_ARG_EQUAL_DESCR(connection_error, 0, "XCB connection to display has failed");

    // Find default screen_id setup
    const xcb_setup_t*    setup = xcb_get_setup(m_env.connection);
    xcb_screen_iterator_t screen_iter = xcb_setup_roots_iterator(setup);
    m_env.screen = screen_iter.data;
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

    // Init window and show on screen
    const Data::FrameSize init_frame_size = InitWindow();

    // Application Initialization
    bool init_success = InitContextWithErrorHandling(m_env, init_frame_size);
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

        // If there's a deferred message, schedule it to show for the current window event loop
        if (HasDeferredMessage())
        {
            ShowAlert(GetDeferredMessage());
            ResetDeferredMessage();
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
    if (!deferred)
    {
        ShowAlert(msg);
    }
}

void AppLin::SetWindowTitle(const std::string& title_text)
{
    META_FUNCTION_TASK();
    Linux::SetXcbWindowStringProperty(m_env.connection, m_env.window, XCB_ATOM_WM_NAME, title_text);
}

bool AppLin::SetFullScreen(bool is_full_screen)
{
    META_FUNCTION_TASK();
    if (!AppBase::SetFullScreen(is_full_screen))
        return false;

    xcb_client_message_event_t msg{0};
    msg.response_type  = XCB_CLIENT_MESSAGE;
    msg.type           = m_state_atom;
    msg.format         = 32;
    msg.window         = m_env.window;
    msg.data.data32[0] = static_cast<uint32_t>(is_full_screen ? Linux::NetWmState::Add : Linux::NetWmState::Remove);
    msg.data.data32[1] = m_state_fullscreen_atom;
    msg.data.data32[2] = XCB_ATOM_NONE;

    Linux::XcbCheck(xcb_send_event_checked(m_env.connection, 1, m_env.window, XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
                                   reinterpret_cast<const char*>(&msg)),
                    m_env.connection, "failed to send full screen state message");

    Data::FrameSize new_size;
    if (is_full_screen)
    {
        m_windowed_frame_size = GetFrameSize();
        new_size.SetWidth(m_env.screen->width_in_pixels);
        new_size.SetHeight(m_env.screen->height_in_pixels);
    }
    else
    {
        new_size = m_windowed_frame_size;
    }

    StartResizing();
    ResizeWindow(new_size, GetPlatformAppSettings().min_size);
    EndResizing();

    return true;
}

void AppLin::Close()
{
    META_FUNCTION_TASK();
    m_is_event_processing = false;
}

void AppLin::ShowAlert(const Message& message)
{
    META_FUNCTION_TASK();
    GetMessageBox().Show(message);
    AppBase::ShowAlert(message);

    if (message.type == Message::Type::Error)
    {
        Close();
    }
}

Data::FrameSize AppLin::InitWindow()
{
    META_FUNCTION_TASK();
    const IApp::Settings& settings = GetPlatformAppSettings();

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
                                  ? m_env.screen->width_in_pixels
                                  : AppBase::GetScaledSize(settings.size.GetWidth(), m_env.screen->width_in_pixels);
    const uint16_t frame_height = settings.is_full_screen
                                  ? m_env.screen->height_in_pixels
                                  : AppBase::GetScaledSize(settings.size.GetHeight(), m_env.screen->height_in_pixels);
    const int16_t pos_x = settings.is_full_screen ? 0 : static_cast<int16_t>(m_env.screen->width_in_pixels - frame_width) / 2;
    const int16_t pos_y = settings.is_full_screen ? 0 : static_cast<int16_t>(m_env.screen->height_in_pixels - frame_height) / 2;

    // Create window and position it in the center of the screen_id
    m_env.window = xcb_generate_id(m_env.connection);
    xcb_create_window(m_env.connection, m_env.screen->root_depth,
                      m_env.window, m_env.screen->root,
                      pos_x, pos_y, frame_width, frame_height, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, m_env.screen->root_visual,
                      value_mask, values.data());

    // Create window delete atom used to receive event when window is destroyed
    m_window_delete_atom = Linux::GetXcbInternAtom(m_env.connection, "WM_DELETE_WINDOW");

    if (settings.is_full_screen)
        Linux::SetXcbWindowAtomProperty<xcb_atom_t>(m_env.connection, m_env.window, m_state_atom, XCB_ATOM_ATOM, { m_window_delete_atom, m_state_fullscreen_atom });
    else
        Linux::SetXcbWindowAtomProperty<xcb_atom_t>(m_env.connection, m_env.window, "WM_PROTOCOLS", XCB_ATOM_ATOM, { m_window_delete_atom });

    // Display application name in window title, dash tooltip and application menu on GNOME and other desktop environment
    SetWindowTitle(settings.name);
    std::string wm_class;
    wm_class = wm_class.insert(0, settings.name);
    wm_class = wm_class.insert(settings.name.size(), 1, '\0');
    wm_class = wm_class.insert(settings.name.size() + 1, settings.name);
    wm_class = wm_class.insert(wm_class.size(), 1, '\0');
    Linux::SetXcbWindowStringProperty(m_env.connection, m_env.window, XCB_ATOM_WM_CLASS, std::string_view(wm_class.c_str(), wm_class.size() + 2));

    m_state_atom            = Linux::GetXcbInternAtom(m_env.connection, "_NET_WM_STATE");
    m_state_hidden_atom     = Linux::GetXcbInternAtom(m_env.connection, "_NET_WM_STATE_HIDDEN");
    m_state_fullscreen_atom = Linux::GetXcbInternAtom(m_env.connection, "_NET_WM_STATE_FULLSCREEN");

    if (settings.icon_provider)
    {
        SetWindowIcon(*settings.icon_provider);
    }

    xcb_map_window(m_env.connection, m_env.window);
    XMoveWindow(m_env.display, m_env.window, pos_x, pos_y);
    xcb_flush(m_env.connection);

    Data::FrameSize frame_size(frame_width, frame_height);
    AppBase::Resize(frame_size, false);
    return frame_size;
}

static void AddIconData(Data::Chunk icon_data, std::vector<uint32_t>& combined_icons_data)
{
    META_FUNCTION_TASK();
    int image_width = 0;
    int image_height = 0;
    int image_channels_count = 0;
    stbi_uc* p_image_data = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(icon_data.GetDataPtr()), // NOSONAR
                                                  static_cast<int>(icon_data.GetDataSize()),
                                                  &image_width, &image_height, &image_channels_count, 4);

    META_CHECK_ARG_NOT_NULL_DESCR(p_image_data, "failed to load image data from memory");
    META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(image_width, 2, "invalid image width");
    META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(image_height, 2, "invalid image height");
    META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(image_channels_count, 3, "invalid image channels count");

    combined_icons_data.reserve(combined_icons_data.size() + 2 + image_width * image_height);
    combined_icons_data.push_back(image_width);
    combined_icons_data.push_back(image_height);

    for(size_t y = 0; y < static_cast<size_t>(image_height); y++)
        for(size_t x = 0; x < static_cast<size_t>(image_width); x++)
        {
            uint32_t bgra_pixel_data = 0;
            uint8_t* bgra_pixel = reinterpret_cast<uint8_t*>(&bgra_pixel_data);
            uint8_t* rgba_pixel = &p_image_data[(y * image_width + x) * 4]; // NOSONAR

            bgra_pixel[0] = rgba_pixel[2];
            bgra_pixel[1] = rgba_pixel[1];
            bgra_pixel[2] = rgba_pixel[0];
            bgra_pixel[3] = rgba_pixel[3];

            combined_icons_data.push_back(bgra_pixel_data);
        }
}

void AppLin::SetWindowIcon(const Data::Provider& icon_provider)
{
    META_FUNCTION_TASK();
    const std::vector<std::string> icon_paths = icon_provider.GetFiles("");
    if (icon_paths.empty())
        return;

    std::vector<uint32_t> combined_icons_data;
    for(const std::string& icon_path : icon_paths)
    {
        AddIconData(icon_provider.GetData(icon_path), combined_icons_data);
    }

    Linux::SetXcbWindowAtomProperty<uint32_t>(m_env.connection, m_env.window, "_NET_WM_ICON", XCB_ATOM_CARDINAL, combined_icons_data);
}

void AppLin::ResizeWindow(const Data::FrameSize& frame_size, const Data::FrameSize& min_size, const Data::Point2I* position)
{
    META_FUNCTION_TASK();
    Linux::WMSizeHints size_hints{0};
    std::vector<uint32_t> config_values;
    uint32_t config_value_mask = 0U;

    size_hints.flags      = PSize | PMinSize;
    size_hints.width      = static_cast<int32_t>(frame_size.GetWidth());
    size_hints.height     = static_cast<int32_t>(frame_size.GetHeight());
    size_hints.min_width  = static_cast<int32_t>(min_size.GetWidth());
    size_hints.min_height = static_cast<int32_t>(min_size.GetHeight());

    if (position)
    {
        size_hints.flags |= PPosition;
        size_hints.x = position->GetX();
        size_hints.y = position->GetY();

        config_value_mask |= XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y;
        config_values.emplace_back(static_cast<uint32_t>(position->GetX()));
        config_values.emplace_back(static_cast<uint32_t>(position->GetY()));
    }

    config_value_mask = XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
    config_values.emplace_back(frame_size.GetWidth());
    config_values.emplace_back(frame_size.GetHeight());

    Linux::SetXcbWindowAtomProperty<Linux::WMSizeHints>(m_env.connection, m_env.window, XCB_ATOM_WM_NORMAL_HINTS, XCB_ATOM_WM_SIZE_HINTS, { size_hints });
    Linux::XcbCheck(xcb_configure_window_checked(m_env.connection, m_env.window, config_value_mask, config_values.data()),
                    m_env.connection, "Failed to configure window size");

    xcb_flush(m_env.connection);
}

void AppLin::HandleEvent(xcb_generic_event_t& event)
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

    case XCB_PROPERTY_NOTIFY:
        OnPropertyChanged(reinterpret_cast<const xcb_property_notify_event_t&>(event));
        break;

    case XCB_MAPPING_NOTIFY:
        OnKeyboardMappingChanged(reinterpret_cast<const xcb_mapping_notify_event_t&>(event));
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
    if (cfg_event.window != m_env.window)
        return;

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
    if (prop_event.atom != m_state_atom || prop_event.window != m_env.window)
        return;

    const std::optional<xcb_atom_t> state_value_opt = Linux::GetXcbWindowPropertyValue<xcb_atom_t>(m_env.connection, m_env.window, m_state_atom);
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
    const Keyboard::Key key = Linux::ConvertXcbKey(m_env.display, m_env.window, key_press_event.detail, key_press_event.state);
    ProcessInputWithErrorHandling(&Input::IActionController::OnKeyboardChanged, key, key_state);
}

void AppLin::OnKeyboardMappingChanged(const xcb_mapping_notify_event_t& mapping_event)
{
    META_FUNCTION_TASK();
    XMappingEvent x_mapping_event{ 0 };
    x_mapping_event.type          = MappingNotify;
    x_mapping_event.send_event    = false;
    x_mapping_event.display       = m_env.display;
    x_mapping_event.window        = m_env.window;
    x_mapping_event.serial        = mapping_event.sequence;
    x_mapping_event.request       = mapping_event.request;
    x_mapping_event.first_keycode = mapping_event.first_keycode;
    x_mapping_event.count         = mapping_event.response_type;
    XRefreshKeyboardMapping(&x_mapping_event);
}

void AppLin::OnMouseButtonChanged(const xcb_button_press_event_t& button_press_event, Mouse::ButtonState button_state)
{
    META_FUNCTION_TASK();
    Mouse::Button button = Mouse::Button::Unknown;
    int delta_sign = -1;
    std::tie(button, delta_sign) = Linux::ConvertXcbMouseButton(button_press_event.detail);

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

MessageBox& AppLin::GetMessageBox()
{
    META_FUNCTION_TASK();
    if (!m_message_box_ptr)
    {
        m_message_box_ptr = std::make_unique<MessageBox>(m_env);
    }
    return *m_message_box_ptr;
}

} // namespace Methane::Platform