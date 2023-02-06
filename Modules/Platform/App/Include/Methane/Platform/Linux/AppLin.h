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
#include <Methane/Platform/Input/Mouse.h>
#include <Methane/Memory.hpp>

#include <vector>
#include <memory>

#include <xcb/xcb.h>
#include <xcb/sync.h>

namespace Methane::Platform
{

class MessageBox;

class AppLin : public AppBase // NOSONAR - this class requires destructor
{
public:
    explicit AppLin(const Settings& settings);
    ~AppLin() override;

    // AppBase interface
    int Run(const RunArgs& args) override;
    void Alert(const Message& msg, bool deferred = false) override;
    void SetWindowTitle(const std::string& title_text) override;
    bool SetFullScreen(bool is_full_screen) override;
    float GetContentScalingFactor() const override;
    uint32_t GetFontResolutionDpi() const override;
    void Close() override;

protected:
    // AppBase interface
    void ShowAlert(const Message& msg) override;

private:
    Data::FrameSize InitWindow();
    void SetWindowIcon(const Data::IProvider& icon_provider);
    void ResizeWindow(const Data::FrameSize& frame_size, const Data::FrameSize& min_size, const Data::Point2I* position = nullptr);
    void HandleEvent(const xcb_generic_event_t& event);
    void OnClientEvent(const xcb_client_message_event_t& event);
    void UpdateSyncCounter();
    void OnWindowConfigured(const xcb_configure_notify_event_t& conf_event);
    void OnPropertyChanged(const xcb_property_notify_event_t& prop_event);
    void OnKeyboardChanged(const xcb_key_press_event_t& key_press_event, Input::Keyboard::KeyState key_state);
    void OnKeyboardMappingChanged(const xcb_mapping_notify_event_t& mapping_event);
    void OnMouseButtonChanged(const xcb_button_press_event_t& button_press_event, Input::Mouse::ButtonState button_state);
    void OnMouseMoved(const xcb_motion_notify_event_t& motion_event);
    void OnMouseInWindowChanged(const xcb_enter_notify_event_t& enter_event, bool mouse_in_window);

    MessageBox& GetMessageBox();

    enum class SyncState
    {
        NotNeeded,
        Received,
        Processed
    };

    AppEnvironment        m_env{ };
    xcb_atom_t            m_protocols_atom        = XCB_ATOM_NONE;
    xcb_atom_t            m_window_delete_atom    = XCB_ATOM_NONE;
    xcb_atom_t            m_sync_request_atom     = XCB_ATOM_NONE;
    xcb_atom_t            m_state_atom            = XCB_ATOM_NONE;
    xcb_atom_t            m_state_hidden_atom     = XCB_ATOM_NONE;
    xcb_atom_t            m_state_fullscreen_atom = XCB_ATOM_NONE;
    bool                  m_is_event_processing   = false;
    bool                  m_is_sync_supported     = false;
    SyncState             m_sync_state            = SyncState::NotNeeded;
    xcb_sync_int64_t      m_sync_value            { 0, 0U };
    xcb_sync_counter_t    m_sync_counter          = 0U;
    UniquePtr<MessageBox> m_message_box_ptr;
    Data::FrameSize       m_windowed_frame_size;
};

} // namespace Methane::Platform
