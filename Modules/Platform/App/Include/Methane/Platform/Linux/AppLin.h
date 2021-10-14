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

FILE: Methane/Platform/Linux/AppLin.h
Linux application implementation.

******************************************************************************/

#pragma once

#include <Methane/Platform/AppBase.h>
#include <Methane/Platform/AppEnvironment.h>
#include <Methane/Platform/Mouse.h>
#include <Methane/Memory.hpp>

#include <vector>
#include <memory>

#include <xcb/xcb.h>

namespace Methane::Platform
{

class MessageBox;

class AppLin : public AppBase
{
public:
    explicit AppLin(const Settings& settings);
    ~AppLin() override;

    // AppBase interface
    int Run(const RunArgs& args) override;
    void Alert(const Message& msg, bool deferred = false) override;
    void SetWindowTitle(const std::string& title_text) override;
    bool SetFullScreen(bool is_full_screen) override;
    void Close() override;

protected:
    // AppBase interface
    void ShowAlert(const Message& msg) override;

private:
    void ResizeWindow(const Data::FrameSize& frame_size, const Data::FrameSize& min_size, const Data::Point2I* position = nullptr);
    void HandleEvent(xcb_generic_event_t& event);
    void OnWindowResized(const xcb_configure_notify_event_t& cfg_event);
    void OnPropertyChanged(const xcb_property_notify_event_t& prop_event);
    void OnKeyboardChanged(const xcb_key_press_event_t& key_press_event, Keyboard::KeyState key_state);
    void OnKeyboardMappingChanged(const xcb_mapping_notify_event_t& mapping_event);
    void OnMouseButtonChanged(const xcb_button_press_event_t& button_press_event, Mouse::ButtonState button_state);
    void OnMouseMoved(const xcb_motion_notify_event_t& motion_event);
    void OnMouseInWindowChanged(const xcb_enter_notify_event_t& enter_event, bool mouse_in_window);

    MessageBox& GetMessageBox();

    AppEnvironment m_env;
    xcb_atom_t m_window_delete_atom = XCB_ATOM_NONE;
    xcb_atom_t m_state_atom = XCB_ATOM_NONE;
    xcb_atom_t m_state_add_atom = XCB_ATOM_NONE;
    xcb_atom_t m_state_remove_atom = XCB_ATOM_NONE;
    xcb_atom_t m_state_hidden_atom = XCB_ATOM_NONE;
    xcb_atom_t m_state_fullscreen_atom = XCB_ATOM_NONE;
    bool m_is_event_processing = false;
    UniquePtr<MessageBox> m_message_box_ptr;
    Data::FrameSize m_windowed_frame_size;
};

} // namespace Methane::Platform
