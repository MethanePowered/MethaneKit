/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: ConsoleApp.h
Console UI application base class implemented using FTXUI framework

******************************************************************************/

#pragma once

#include <Methane/Data/Types.h>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <mutex>
#include <atomic>

namespace Methane::Tutorials
{

namespace data = Methane::Data;

class ConsoleApp
{
public:
    ConsoleApp() = default;
    virtual ~ConsoleApp() = default;

    const data::FrameSize& GetFieldSize() const        { return m_field_size; }
    const data::FrameRect& GetVisibleFrameRect() const { return m_frame_rect; }

    double GetInitialCellsRatio() const   { return static_cast<double>(m_initial_cells_percent) / 100.0; }
    bool   IsScreenRefreshEnabled() const { return m_screen_refresh_enabled; }
    void   ToggleScreenRefresh();

    // ConsoleApp virtual interface
    virtual int Run();
    virtual std::string_view                GetGraphicsApiName() const = 0;
    virtual const std::string&              GetComputeDeviceName() const = 0;
    virtual const std::vector<std::string>& GetComputeDeviceNames() const = 0;
    virtual uint32_t                        GetFramesCountPerSecond() const = 0;
    virtual uint32_t                        GetVisibleCellsCount() const = 0;

protected:
    virtual void Init() = 0;
    virtual void Release() = 0;
    virtual void Compute() = 0;
    virtual void Present(ftxui::Canvas& canvas) = 0;
    virtual void Restart() = 0;

    int GetComputeDeviceIndex() const   { return m_compute_device_index; }
    std::mutex& GetScreenRefreshMutex() { return m_screen_refresh_mutex; }

    void InitUserInterface();

private:
    void UpdateFrameSize(int width, int height);
    bool HandleInputEvent(ftxui::Event& e);

    ftxui::ScreenInteractive     m_screen{ ftxui::ScreenInteractive::Fullscreen() };
    ftxui::RadioboxOption        m_compute_device_option{ ftxui::RadioboxOption::Simple() };
    ftxui::Component             m_root;
    std::mutex                   m_screen_refresh_mutex;
    std::atomic<bool>            m_screen_refresh_enabled{ true };
    bool                         m_30fps_screen_refresh_limit_enabled{ true };
    int                          m_compute_device_index = 0;
    data::FrameSize              m_field_size{ 2048U, 2048U };
    data::FrameRect              m_frame_rect;
    std::optional<data::Point2I> m_mouse_pressed_pos;
    std::optional<data::Point2I> m_frame_pressed_pos;
    int                          m_initial_cells_percent = 50U;
};

} // namespace Methane::Tutorials
