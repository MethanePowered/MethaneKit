/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Platform/Windows/AppWin.h
Windows application implementation.

******************************************************************************/

#pragma once

#include <Methane/Platform/AppBase.h>
#include <Methane/Platform/AppEnvironment.h>
#include <Methane/Platform/Mouse.h>

#include <windows.h>

#include <vector>
#include <memory>

namespace Methane
{
namespace Platform
{

class AppWin : public AppBase
{
public:
    AppWin(const Settings& settings);

    // AppBase interface
    int Run(const RunArgs& args) override;
    void Alert(const Message& msg, bool deferred = false) override;
    void SetWindowTitle(const std::string& title_text) override;
    bool SetFullScreen(bool is_full_screen) override;
    void Close() override;

protected:
    // AppBase interface
    void ParseCommandLine(const cxxopts::ParseResult& cmd_parse_result) override;
    void ShowAlert(const Message& msg) override;
    
    void ScheduleAlert();

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    AppEnvironment m_env;

private:
    Mouse::State m_mouse_state;
    RECT         m_window_rect = {};
};

} // namespace Platform
} // namespace Methane
