/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
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

#include <windowsx.h>
#include <nowide/convert.hpp>

#include <cassert>

namespace Methane::Platform
{

constexpr auto WM_ALERT = WM_USER + 1;

static const wchar_t* g_window_class = L"MethaneWindowClass";
static const wchar_t* g_window_icon  = L"IDI_APP_ICON";

static UINT ConvertMessageTypeToFlags(AppBase::Message::Type msg_type)
{
    ITT_FUNCTION_TASK();
    switch (msg_type)
    {
    case AppBase::Message::Type::Information:   return MB_ICONINFORMATION | MB_OK;
    case AppBase::Message::Type::Warning:       return MB_ICONWARNING | MB_OK;
    case AppBase::Message::Type::Error:         return MB_ICONERROR | MB_OK;
    }
    return 0;
}

AppWin::AppWin(const AppBase::Settings& settings)
    : AppBase(settings)
{
    ITT_FUNCTION_TASK();
}

int AppWin::Run(const RunArgs& args)
{
    // Skip instrumentation ITT_FUNCTION_TASK() since this is the only root function running till application close

    const int base_return_code = AppBase::Run(args);
    if (base_return_code)
        return base_return_code;

    // Initialize the window class.
    WNDCLASSEX window_class     = { };
    window_class.cbSize         = sizeof(WNDCLASSEX);
    window_class.style          = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc    = WindowProc;
    window_class.hInstance      = GetModuleHandle(nullptr);
    window_class.hCursor        = LoadCursor(NULL, IDC_ARROW);
    window_class.lpszClassName  = g_window_class;
    window_class.hIcon          = LoadIcon(window_class.hInstance, g_window_icon);

    RegisterClassEx(&window_class);

    uint32_t desktop_width = 0, desktop_height = 0;
    Methane::Platform::Windows::GetDesktopResolution(desktop_width, desktop_height);

    const Settings& app_settings = GetPlatformAppSettings();

    Data::FrameSize frame_size;
    frame_size.width  = app_settings.width < 1.0  ? static_cast<uint32_t>(desktop_width * (app_settings.width > 0.0 ? app_settings.width : 0.7))
                                                  : static_cast<uint32_t>(app_settings.width);
    frame_size.height = app_settings.height < 1.0 ? static_cast<uint32_t>(desktop_height * (app_settings.height > 0.0 ? app_settings.height : 0.7))
                                                  : static_cast<uint32_t>(app_settings.height);

    RECT window_rect = { 0, 0, static_cast<LONG>(frame_size.width), static_cast<LONG>(frame_size.height) };
    AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);
    const Data::FrameSize window_size(static_cast<uint32_t>(window_rect.right - window_rect.left),
                                      static_cast<uint32_t>(window_rect.bottom - window_rect.top));

    // Create the window and store a handle to it.
    m_env.window_handle = CreateWindowEx(NULL,
        g_window_class,
        nowide::widen(app_settings.name).c_str(),
        WS_OVERLAPPEDWINDOW,
        (desktop_width - window_size.width) / 2,
        (desktop_height - window_size.height) / 2,
        window_size.width,
        window_size.height,
        NULL, // No parent window
        NULL, // No menus
        window_class.hInstance,
        this);

    ShowWindow(m_env.window_handle, SW_SHOW);

    // If there's a deferred message, schedule it to show for the current window message loop
    if (m_sp_deferred_message)
    {
        ScheduleAlert();
    }

    bool init_failed = false;
#ifndef _DEBUG
    try
    {
#endif
        InitContext(m_env, frame_size);
        Init();
#ifndef _DEBUG
    }
    catch (std::exception& e)
    {
        init_failed = true;
        Alert({ Message::Type::Error, "Application Initialization Error", e.what() });
    }
    catch (...)
    {
        init_failed = true;
        Alert({ Message::Type::Error, "Application Initialization Error", "Unknown exception occurred." });
    }
#endif

    // Main message loop
    MSG msg = { 0 };
    while (true)
    {
        // Process any messages in the queue.
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
                break;
        }

        if (init_failed)
            continue;

#ifndef _DEBUG
        try
        {
#endif
            UpdateAndRender();
#ifndef _DEBUG
        }
        catch (std::exception& e)
        {
            Alert({ Message::Type::Error, "Application Render Error", e.what() });
        }
        catch (...)
        {
            Alert({ Message::Type::Error, "Application Render Error", "Unknown exception occurred." });
        }
#endif
    }

    // Return this part of the WM_QUIT message to Windows.
    return static_cast<char>(msg.wParam);
}

void AppWin::Alert(const Message& msg, bool deferred)
{
    ITT_FUNCTION_TASK();
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

LRESULT CALLBACK AppWin::WindowProc(HWND h_wnd, UINT msg_id, WPARAM w_param, LPARAM l_param)
{
    ITT_FUNCTION_TASK();

    AppWin* p_app = reinterpret_cast<AppWin*>(GetWindowLongPtr(h_wnd, GWLP_USERDATA));

#ifndef _DEBUG
    try
    {
#endif
        switch (msg_id)
        {
        case WM_CREATE:
        {
            // Save the DXSample* passed in to CreateWindow.
            LPCREATESTRUCT p_create_struct = reinterpret_cast<LPCREATESTRUCT>(l_param);
            SetWindowLongPtr(h_wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(p_create_struct->lpCreateParams));
        }
        return 0;

        case WM_DESTROY:
        {
            PostQuitMessage(0);
        }
        return 0;

        case WM_SIZE:
        {
            if (p_app)
            {
                RECT window_rect = {};
                GetWindowRect(h_wnd, &window_rect);
                p_app->ChangeWindowBounds(Data::FrameRect {
                    Data::Point2i(window_rect.left, window_rect.top),
                    Data::FrameSize(static_cast<uint32_t>(window_rect.right  - window_rect.left),
                              static_cast<uint32_t>(window_rect.bottom - window_rect.top))
                });

                RECT client_rect = {};
                GetClientRect(h_wnd, &client_rect);
                p_app->Resize(Data::FrameSize(
                    static_cast<uint32_t>(client_rect.right  - client_rect.left),
                    static_cast<uint32_t>(client_rect.bottom - client_rect.top)),
                    w_param == SIZE_MINIMIZED);
            }
        }
        break;

        case WM_ALERT:
        {
            assert(!!p_app->m_sp_deferred_message);
            if (p_app->m_sp_deferred_message)
            {
                p_app->ShowAlert(*p_app->m_sp_deferred_message);
            }
        }
        break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            const Keyboard::Key      key       = Keyboard::KeyConverter({ w_param, l_param }).GetKey();
            const Keyboard::KeyState key_state = ((l_param >> 31) & 1) ? Keyboard::KeyState::Released : Keyboard::KeyState::Pressed;

            if (key == Keyboard::Key::Unknown)
                break;

            if (key_state == Keyboard::KeyState::Released && w_param == VK_SHIFT)
            {
                // HACK: Release both Shift keys on Shift up event, as when both
                //       are pressed the first release does not emit any event
                p_app->InputState().OnKeyboardChanged(Keyboard::Key::LeftShift, key_state);
                p_app->InputState().OnKeyboardChanged(Keyboard::Key::RightShift, key_state);
            }
            else if (w_param == VK_SNAPSHOT)
            {
                // HACK: Key down is not reported for the Print Screen key
                p_app->InputState().OnKeyboardChanged(key, Keyboard::KeyState::Pressed);
                p_app->InputState().OnKeyboardChanged(key, Keyboard::KeyState::Released);
            }
            else
            {
                p_app->InputState().OnKeyboardChanged(key, key_state);
            }
        } break;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        {
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

            if (p_app->m_mouse_state.GetPressedButtons().empty())
            {
                SetCapture(h_wnd);
            }

            p_app->m_mouse_state.SetButton(button, button_state);
            p_app->InputState().OnMouseButtonChanged(button, button_state);

            if (p_app->m_mouse_state.GetPressedButtons().empty())
            {
                ReleaseCapture();
            }

            if (msg_id == WM_XBUTTONDOWN || msg_id == WM_XBUTTONUP)
                return TRUE;

            return 0;
        }

        case WM_MOUSEMOVE:
        {
            const int x = GET_X_LPARAM(l_param);
            const int y = GET_Y_LPARAM(l_param);

            p_app->InputState().OnMousePositionChanged({ x, y });

            if (!p_app->GetInputState().GetMouseState().IsInWindow())
            {
                // Subscribe window to track WM_MOUSELEAVE
                TRACKMOUSEEVENT tme;
                ZeroMemory(&tme, sizeof(tme));
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = h_wnd;
                TrackMouseEvent(&tme);

                p_app->InputState().OnMouseInWindowChanged(true);
            }

            return 0;
        }

        case WM_MOUSELEAVE:
        {
            p_app->InputState().OnMouseInWindowChanged(false);
            return 0;
        }

        case WM_MOUSEWHEEL:
        {
            const float wheel_delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(w_param)) / WHEEL_DELTA;
            p_app->InputState().OnMouseScrollChanged({ 0.f, wheel_delta });
            return 0;
        }

        case WM_MOUSEHWHEEL: // Windows Vista and later
        {
            // NOTE: The X-axis is inverted for consistency with macOS and X11
            const float wheel_delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(w_param)) / WHEEL_DELTA;
            p_app->InputState().OnMouseScrollChanged({ -wheel_delta, 0.f });
            return 0;
        }

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
    ITT_FUNCTION_TASK();

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
    ITT_FUNCTION_TASK();
    if (!m_env.window_handle)
        return;

    const BOOL post_result = PostMessage(m_env.window_handle, WM_ALERT, 0, 0);
    assert(post_result != 0);
}

void AppWin::SetWindowTitle(const std::string& title_text)
{
    ITT_FUNCTION_TASK();
    assert(!!m_env.window_handle);

    BOOL set_result = SetWindowTextW(m_env.window_handle, nowide::widen(title_text).c_str());
    assert(set_result);
}

bool AppWin::SetFullScreen(bool is_full_screen)
{
    ITT_FUNCTION_TASK();
    if (!AppBase::SetFullScreen(is_full_screen))
        return false;

    assert(!!m_env.window_handle);
    
    RECT            window_rect     = {};
    int32_t         window_style    = WS_OVERLAPPEDWINDOW;
    int32_t         window_mode     = 0;
    HWND            window_position = nullptr;
    const Settings& app_settings    = GetPlatformAppSettings();

    if (app_settings.is_full_screen)
    {
        GetWindowRect(m_env.window_handle, &m_window_rect);

        window_style   &= ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME);
        window_position = HWND_TOPMOST;
        window_mode     = SW_MAXIMIZE;

        // Get resolution and location of the monitor where current window is located
        HMONITOR    monitor_handle = MonitorFromWindow(m_env.window_handle, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitor_info   = {};
        monitor_info.cbSize        = sizeof(MONITORINFO);
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
    ITT_FUNCTION_TASK();
    if (m_env.window_handle)
    {
        BOOL post_result = PostMessage(m_env.window_handle, WM_CLOSE, 0, 0);
        assert(post_result != 0);
    }
    else
    {
        ExitProcess(0);
    }
}

} // namespace Methane::Platform