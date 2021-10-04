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

#include <vector>
#include <memory>

#include <xcb/xcb.h>

namespace Methane::Platform
{

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
    void ScheduleAlert();
    void HandleEvent(xcb_generic_event_t& event);
    void OnWindowResized(const xcb_configure_notify_event_t& cfg_event);
    void OnPropertyChanged(const xcb_property_notify_event_t& prop_event);

    AppEnvironment m_env;
    xcb_atom_t m_window_delete_atom = XCB_ATOM_NONE;
    xcb_atom_t m_state_atom = XCB_ATOM_NONE;
    xcb_atom_t m_state_hidden_atom = XCB_ATOM_NONE;
    xcb_atom_t m_state_fullscreen_atom = XCB_ATOM_NONE;
    bool m_is_event_processing = false;
};

} // namespace Methane::Platform
