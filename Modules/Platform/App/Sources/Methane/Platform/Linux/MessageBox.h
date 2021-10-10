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

FILE: Methane/Platform/Linux/MessageBox.h
Linux message box implementation with X11/XCB.

******************************************************************************/

#pragma once

#include <Methane/Platform/IApp.h>
#include <Methane/Platform/AppEnvironment.h>
#include <Methane/Platform/Mouse.h>

#include <xcb/xcb.h>

namespace Methane::Platform
{

class MessageBox
{
public:
    explicit MessageBox(const AppEnvironment& app_env);
    ~MessageBox();

    void Show(const IApp::Message& message);

private:
    void ScheduleAlert();
    void HandleEvent(xcb_generic_event_t& event);
    void OnWindowResized(const xcb_configure_notify_event_t& cfg_event);
    void OnKeyboardChanged(const xcb_key_press_event_t& key_press_event, bool is_key_pressed);
    void OnMouseButtonChanged(const xcb_button_press_event_t& button_press_event, bool is_button_pressed);

    const AppEnvironment m_app_env;
    IApp::Message m_message;
    xcb_window_t m_dialog_window = 0U;
    xcb_atom_t m_window_delete_atom = XCB_ATOM_NONE;
    bool m_is_event_processing = false;
};

} // namespace Methane::Platform
