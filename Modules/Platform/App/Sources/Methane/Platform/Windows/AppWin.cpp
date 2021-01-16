/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Platform/Windows/AppWin.cpp
Windows application implementation.

******************************************************************************/

#include <Methane/Platform/Windows/AppWin.h>
#include <Methane/Platform/Utils.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <windowsx.h>
#include <nowide/convert.hpp>

namespace Methane::Platform
{

constexpr auto WM_ALERT = WM_USER + 1;

static const wchar_t* const g_window_class = L"MethaneWindowClass";
static const wchar_t* const g_window_icon  = L"IDI_APP_ICON";

static UINT ConvertMessageTypeToFlags(AppBase::Message::Type msg_type)
{
    META_FUNCTION_TASK();
    switch (msg_type)
    {
    case AppBase::Message::Type::Information:   return MB_ICONINFORMATION | MB_OK;
    case AppBase::Message::Type::Warning:       return MB_ICONWARNING | MB_OK;
    case AppBase::Message::Type::Error:         return MB_ICONERROR | MB_OK;
    default:                                    META_UNEXPECTED_ARG_RETURN(msg_type, 0);
    }
}

AppWin::AppWin(const AppBase::Settings& settings)
    : AppBase(settings)
{
    META_FUNCTION_TASK();
}

int AppWin::Run(const RunArgs& args)
{
    // Skip instrumentation META_FUNCTION_TASK() since this is the only root function running till application close
    if (const int base_return_code = AppBase::Run(args);
        base_return_code)
        return base_return_code;

    // Initialize the window class.
    WNDCLASSEX window_class{};
    window_class.cbSize         = sizeof(WNDCLASSEX);
    window_class.style          = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc    = WindowProc;
    window_class.hInstance      = GetModuleHandle(nullptr);
    window_class.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    window_class.lpszClassName  = g_window_class;
    window_class.hIcon          = LoadIcon(window_class.hInstance, g_window_icon);

    RegisterClassEx(&window_class);

    uint32_t desktop_width  = 0U;
    uint32_t desktop_height = 0U;
    Windows::GetDesktopResolution(desktop_width, desktop_height);

    const Settings& app_settings = GetPlatformAppSettings();

    Data::FrameSize frame_size;
    frame_size.width  = app_settings.width < 1.0
                      ? static_cast<uint32_t>(desktop_width * app_settings.width)
                      : static_cast<uint32_t>(app_settings.width);
    frame_size.height = app_settings.height < 1.0
                      ? static_cast<uint32_t>(desktop_height * app_settings.height)
                      : static_cast<uint32_t>(app_settings.height);

    RECT window_rect{ 0, 0, static_cast<LONG>(frame_size.width), static_cast<LONG>(frame_size.height) };
    AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);
    const Data::FrameSize window_size(static_cast<uint32_t>(window_rect.right - window_rect.left),
                                      static_cast<uint32_t>(window_rect.bottom - window_rect.top));

    // Create the window and store a handle to it.
    m_env.window_handle = CreateWindowEx(0,
        g_window_class,
        nowide::widen(app_settings.name).c_str(),
        WS_OVERLAPPEDWINDOW,
        (desktop_width - window_size.width) / 2,
        (desktop_height - window_size.height) / 2,
        window_size.width,
        window_size.height,
        nullptr, // No parent window
        nullptr, // No menus
        window_class.hInstance,
        this);

    ShowWindow(m_env.window_handle, SW_SHOW);

    // If there's a deferred message, schedule it to show for the current window message loop
    if (HasDeferredMessage())
    {
        ScheduleAlert();
    }

    // Application Initialization
    bool init_success = InitContextWithErrorHandling(m_env, frame_size);
    if (init_success)
    {
        init_success = InitWithErrorHandling();
    }

    // Main message loop
    MSG msg{};
    while (m_is_message_processing)
    {
        // Process any messages in the queue.
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
                break;
        }

        if (!init_success || !m_is_message_processing)
            continue;

        UpdateAndRenderWithErrorHandling();
    }

    // Return this part of the WM_QUIT message to Windows.
    return static_cast<char>(msg.wParam);
}

void AppWin::Alert(const Message& msg, bool deferred)
{
    META_FUNCTION_TASK();
    AppBase::Alert(msg, deferred);

    if (deferred)
    {
        ScheduleAlert();
    }
    else
    {
        ShowAlert(msg);
    }
}

void AppWin::OnWindowAlert()
{
    META_FUNCTION_TASK();
    if (!HasDeferredMessage())
        return;

    ShowAlert(GetDeferredMessage());
    ResetDeferredMessage();
}

LRESULT AppWin::OnWindowDestroy()
{
    META_FUNCTION_TASK();
    StopMessageProcessing();
    PostQuitMessage(0);
    return 0;
}

void AppWin::OnWindowResized(WPARAM w_param, LPARAM l_param)
{
    META_FUNCTION_TASK();
    META_UNUSED(l_param);

    RECT window_rect{};
    GetWindowRect(m_env.window_handle, &window_rect);
    ChangeWindowBounds(
        {
            Data::Point2i(window_rect.left, window_rect.top),
            Data::FrameSize(static_cast<uint32_t>(window_rect.right - window_rect.left),
                            static_cast<uint32_t>(window_rect.bottom - window_rect.top))
        }
    );

    RECT client_rect{};
    GetClientRect(m_env.window_handle, &client_rect);
    Resize(
        {
            static_cast<uint32_t>(client_rect.right - client_rect.left),
            static_cast<uint32_t>(client_rect.bottom - client_rect.top)
        },
        w_param == SIZE_MINIMIZED
    );

    if (IsResizing())
    {
        UpdateAndRenderWithErrorHandling();
    }
}

LRESULT AppWin::OnWindowResizing(WPARAM w_param, LPARAM l_param)
{
    META_FUNCTION_TASK();
    META_UNUSED(w_param);

    RECT  window_rect{};
    GetWindowRect(m_env.window_handle, &window_rect);

    RECT  client_rect{};
    GetClientRect(m_env.window_handle, &client_rect);

    int border = (window_rect.right - window_rect.left) - client_rect.right;
    int header = (window_rect.bottom - window_rect.top) - client_rect.bottom;

    
    const Settings& settings  = GetPlatformAppSettings();
    int32_t min_window_width  = settings.min_width + border;
    int32_t min_window_height = settings.min_height + header;

    // Update window rectangle with respect to minimum size limit
    auto p_window_rect = reinterpret_cast<PRECT>(l_param);

    if (p_window_rect->right - p_window_rect->left < min_window_width)
    {
        if (w_param == WMSZ_RIGHT || w_param == WMSZ_BOTTOMRIGHT || w_param == WMSZ_TOPRIGHT)
            p_window_rect->right = p_window_rect->left + min_window_width;
        else
            p_window_rect->left  = p_window_rect->right - min_window_width;
    }

    if (p_window_rect->bottom - p_window_rect->top < min_window_height)
    {
        if (w_param == WMSZ_BOTTOM || w_param == WMSZ_BOTTOMLEFT || w_param == WMSZ_BOTTOMRIGHT)
            p_window_rect->bottom = p_window_rect->top + min_window_height;
        else
            p_window_rect->top    = p_window_rect->bottom - min_window_height;
    }

    return TRUE;
}

void AppWin::OnWindowKeyboardEvent(WPARAM w_param, LPARAM l_param)
{
    META_FUNCTION_TASK();

    const Keyboard::Key      key = Keyboard::KeyConverter({ w_param, l_param }).GetKey();
    const Keyboard::KeyState key_state = ((l_param >> 31) & 1) ? Keyboard::KeyState::Released : Keyboard::KeyState::Pressed;

    if (key == Keyboard::Key::Unknown)
        return;

    if (key_state == Keyboard::KeyState::Released && w_param == VK_SHIFT)
    {
        // HACK: Release both Shift keys on Shift up event, as when both
        //       are pressed the first release does not emit any event
        ProcessInputWithErrorHandling(&Input::IActionController::OnKeyboardChanged, Keyboard::Key::LeftShift, key_state);
        ProcessInputWithErrorHandling(&Input::IActionController::OnKeyboardChanged, Keyboard::Key::RightShift, key_state);
    }
    else if (w_param == VK_SNAPSHOT)
    {
        // HACK: Key down is not reported for the Print Screen key
        ProcessInputWithErrorHandling(&Input::IActionController::OnKeyboardChanged, key, Keyboard::KeyState::Pressed);
        ProcessInputWithErrorHandling(&Input::IActionController::OnKeyboardChanged, key, Keyboard::KeyState::Released);
    }
    else
    {
        ProcessInputWithErrorHandling(&Input::IActionController::OnKeyboardChanged, key, key_state);
    }
}

LRESULT AppWin::OnWindowMouseButtonEvent(UINT msg_id, WPARAM w_param, LPARAM l_param)
{
    META_FUNCTION_TASK();
    META_UNUSED(l_param);

    Mouse::Button button = Mouse::Button::Unknown;
    if (msg_id == WM_LBUTTONDOWN || msg_id == WM_LBUTTONUP)
        button = Mouse::Button::Left;
    else if (msg_id == WM_RBUTTONDOWN || msg_id == WM_RBUTTONUP)
        button = Mouse::Button::Right;
    else if (msg_id == WM_MBUTTONDOWN || msg_id == WM_MBUTTONUP)
        button = Mouse::Button::Middle;
    else if (GET_XBUTTON_WPARAM(w_param) == XBUTTON1)
        button = Mouse::Button::Button4;
    else
        button = Mouse::Button::Button5;

    const Mouse::ButtonState button_state = (msg_id == WM_LBUTTONDOWN || msg_id == WM_RBUTTONDOWN ||
        msg_id == WM_MBUTTONDOWN || msg_id == WM_XBUTTONDOWN)
        ? Mouse::ButtonState::Pressed : Mouse::ButtonState::Released;

    if (m_mouse_state.GetPressedButtons().empty())
    {
        SetCapture(m_env.window_handle);
    }

    m_mouse_state.SetButton(button, button_state);
    ProcessInputWithErrorHandling(&Input::IActionController::OnMouseButtonChanged, button, button_state);

    if (m_mouse_state.GetPressedButtons().empty())
    {
        ReleaseCapture();
    }

    if (msg_id == WM_XBUTTONDOWN || msg_id == WM_XBUTTONUP)
        return TRUE;

    return FALSE;
}

LRESULT AppWin::OnWindowMouseMoveEvent(WPARAM w_param, LPARAM l_param)
{
    META_FUNCTION_TASK();
    META_UNUSED(w_param);

    const int x = GET_X_LPARAM(l_param);
    const int y = GET_Y_LPARAM(l_param);

    ProcessInputWithErrorHandling(&Input::IActionController::OnMousePositionChanged, Mouse::Position{ x, y });

    if (!GetInputState().GetMouseState().IsInWindow())
    {
        // Subscribe window to track WM_MOUSELEAVE
        TRACKMOUSEEVENT tme;
        ZeroMemory(&tme, sizeof(tme));
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_env.window_handle;
        TrackMouseEvent(&tme);

        ProcessInputWithErrorHandling(&Input::IActionController::OnMouseInWindowChanged, true);
    }

    return 0;
}

LRESULT AppWin::OnWindowMouseWheelEvent(bool is_vertical_scroll, WPARAM w_param, LPARAM l_param)
{
    META_FUNCTION_TASK();
    META_UNUSED(w_param);
    META_UNUSED(l_param);

    if (is_vertical_scroll)
    {
        const float wheel_delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(w_param)) / WHEEL_DELTA;
        ProcessInputWithErrorHandling(&Input::IActionController::OnMouseScrollChanged, Mouse::Scroll{ 0.F, wheel_delta });
    }
    else
    {
        // NOTE: The X-axis is inverted for consistency with macOS and X11
        const float wheel_delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(w_param)) / WHEEL_DELTA;
        ProcessInputWithErrorHandling(&Input::IActionController::OnMouseScrollChanged, Mouse::Scroll{ -wheel_delta, 0.F });
    }

    return 0;
}

LRESULT AppWin::OnWindowMouseLeave()
{
    META_FUNCTION_TASK();
    ProcessInputWithErrorHandling(&Input::IActionController::OnMouseInWindowChanged, false);
    return 0;
}

LRESULT CALLBACK AppWin::WindowProc(HWND h_wnd, UINT msg_id, WPARAM w_param, LPARAM l_param)
{
    META_FUNCTION_TASK();

    if (msg_id == WM_CREATE)
    {
        SetWindowLongPtr(h_wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(reinterpret_cast<LPCREATESTRUCT>(l_param)->lpCreateParams));
        return 0;
    }

    auto p_app = reinterpret_cast<AppWin*>(GetWindowLongPtr(h_wnd, GWLP_USERDATA));
    if (!p_app || !p_app->IsMessageProcessing())
    {
        return DefWindowProc(h_wnd, msg_id, w_param, l_param);
    }

#ifndef _DEBUG
    try
    {
#endif
        switch (msg_id)
        {
        case WM_ALERT:          p_app->OnWindowAlert(); break;
        case WM_DESTROY:        return p_app->OnWindowDestroy();

        // Windows resizing events
        case WM_ENTERSIZEMOVE:  p_app->StartResizing(); break;
        case WM_EXITSIZEMOVE:   p_app->EndResizing(); break;
        case WM_SIZING:         return p_app->OnWindowResizing(w_param, l_param);
        case WM_SIZE:           p_app->OnWindowResized(w_param, l_param); break;
        
        // Keyboard events
        case WM_SETFOCUS:       p_app->SetKeyboardFocus(true);  break;
        case WM_KILLFOCUS:      p_app->SetKeyboardFocus(false); break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:       p_app->OnWindowKeyboardEvent(w_param, l_param); break;

        // Mouse events
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:      return p_app->OnWindowMouseButtonEvent(msg_id, w_param, l_param);
        case WM_MOUSEMOVE:      return p_app->OnWindowMouseMoveEvent(w_param, l_param);
        case WM_MOUSEWHEEL:     return p_app->OnWindowMouseWheelEvent(true, w_param, l_param);
        case WM_MOUSEHWHEEL:    return p_app->OnWindowMouseWheelEvent(false, w_param, l_param);
        case WM_MOUSELEAVE:     return p_app->OnWindowMouseLeave();

        default: break;
        }
#ifndef _DEBUG
    }
    catch (std::exception& e)
    {
        p_app->Alert({ Message::Type::Error, "Application Input Error", e.what() });
    }
    catch (...)
    {
        p_app->Alert({ Message::Type::Error, "Application Input Error", "Unknown exception occurred." });
    }
#endif

    // Handle any messages the switch statement didn't.
    return DefWindowProc(h_wnd, msg_id, w_param, l_param);
}

void AppWin::ShowAlert(const Message& msg)
{
    META_FUNCTION_TASK();

    MessageBox(
        m_env.window_handle,
        nowide::widen(msg.information).c_str(),
        nowide::widen(msg.title).c_str(),
        ConvertMessageTypeToFlags(msg.type)
    );

    AppBase::ShowAlert(msg);

    if (msg.type == Message::Type::Error)
    {
        Close();
    }
}

void AppWin::ScheduleAlert()
{
    META_FUNCTION_TASK();
    if (!m_env.window_handle)
        return;

    const BOOL post_result = PostMessage(m_env.window_handle, WM_ALERT, 0, 0);
    META_CHECK_ARG_TRUE_DESCR(post_result, "failed to post window message");
}

void AppWin::SetWindowTitle(const std::string& title_text)
{
    META_FUNCTION_TASK();
    if (!m_env.window_handle)
        return;

    BOOL set_result = SetWindowTextW(m_env.window_handle, nowide::widen(title_text).c_str());
    META_CHECK_ARG_TRUE_DESCR(set_result, "failed to update window title");
}

bool AppWin::SetFullScreen(bool is_full_screen)
{
    META_FUNCTION_TASK();
    if (!AppBase::SetFullScreen(is_full_screen))
        return false;

    META_CHECK_ARG_NOT_NULL(m_env.window_handle);
    
    RECT    window_rect{};
    int32_t window_style    = WS_OVERLAPPEDWINDOW;
    int32_t window_mode     = 0;
    HWND    window_position = nullptr;

    if (GetPlatformAppSettings().is_full_screen)
    {
        GetWindowRect(m_env.window_handle, &m_window_rect);

        window_style   &= ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME);
        window_position = HWND_TOPMOST;
        window_mode     = SW_MAXIMIZE;

        // Get resolution and location of the monitor where current window is located
        HMONITOR    monitor_handle = MonitorFromWindow(m_env.window_handle, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitor_info{};
        monitor_info.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo(monitor_handle, &monitor_info);
        window_rect = monitor_info.rcMonitor;
    }
    else
    {
        window_rect     = m_window_rect;
        window_position = HWND_NOTOPMOST;
        window_mode     = SW_NORMAL;
    }

    SetWindowLong(m_env.window_handle, GWL_STYLE, window_style);
    SetWindowPos(m_env.window_handle, window_position,
                 window_rect.left,    window_rect.top,
                 window_rect.right  - window_rect.left,
                 window_rect.bottom - window_rect.top,
                 SWP_FRAMECHANGED | SWP_NOACTIVATE);

    ShowWindow(m_env.window_handle, window_mode);
    return true;
}

void AppWin::Close()
{
    META_FUNCTION_TASK();
    if (m_env.window_handle && PostMessage(m_env.window_handle, WM_CLOSE, 0, 0))
        return;

    ExitProcess(0);
}

} // namespace Methane::Platform