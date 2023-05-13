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

FILE: ConsoleApp.cpp
Console UI application base class implemented using FTXUI framework

******************************************************************************/

#include "ConsoleApp.h"

#include <Methane/Version.h>
#include <Methane/Instrumentation.h>

#include <fmt/format.h>
#include <thread>

namespace Methane::Tutorials
{

ConsoleApp::ConsoleApp()
    : m_screen(ftxui::ScreenInteractive::Fullscreen())
    , m_compute_device_option(ftxui::RadioboxOption::Simple())
{
}

int ConsoleApp::Run()
{
    META_FUNCTION_TASK();
    std::atomic<bool> refresh_ui_continue = true;
    std::thread refresh_ui([this, &refresh_ui_continue]
    {
        uint32_t time = 0;
        std::condition_variable_any update_condition_var;
        while (refresh_ui_continue)
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(m_30fps_screen_refresh_limit_enabled ? 32ms : 1ms);
            std::unique_lock lock(m_screen_refresh_mutex);
            update_condition_var.wait_for(lock, 1s, [this] { return m_screen_refresh_enabled.load(); });
            m_screen.Post([&time] { time++; });
            m_screen.Post(ftxui::Event::Custom);
        }
    });

    m_screen.Loop(m_root);
    refresh_ui_continue = false;
    refresh_ui.join();
    return 0;
}

void ConsoleApp::ToggleScreenRefresh()
{
    m_screen_refresh_enabled = !m_screen_refresh_enabled;
}

void ConsoleApp::InitUserInterface()
{
    META_FUNCTION_TASK();
    using namespace ftxui;
    auto toolbar = Container::Horizontal({
        Renderer([this]
        {
            return hbox({
                text(fmt::format(" API: {} ", GetGraphicsApiName())),
                separator(),
                text(fmt::format(" GPU: {} ", GetComputeDeviceName())),
                separator(),
                text(fmt::format(" FPS: {} ", GetFramesCountPerSecond())),
                separator(),
                text(fmt::format(" Field: {} x {} ", m_frame_size.GetWidth(), m_frame_size.GetHeight())),
                separator(),
                text(fmt::format(" Visible {} ", static_cast<std::string>(m_frame_rect) )),
                separator(),
                text(fmt::format(" Visible Cells {} ", GetVisibleCellsCount()))
            });
        }) | border | xflex,
        Button(" X ", m_screen.ExitLoopClosure(), ButtonOption::Simple()) | align_right
    });

    m_compute_device_option.on_change = [this]()
    {
        Release();
        Init();
    };

    auto sidebar = Container::Vertical({
        Renderer([&]{ return text("GPU Devices:") | ftxui::bold; }),
        Radiobox(&GetComputeDeviceNames(), &m_compute_device_index, m_compute_device_option),
        Renderer([&] { return separator(); }),
        Checkbox("30 FPS limit", &m_30fps_screen_refresh_limit_enabled),
        Container::Horizontal({
            Button("Restart",      [this]() { Restart(); },             ButtonOption::Border()),
            Button("Play | Pause", [this]() { ToggleScreenRefresh(); }, ButtonOption::Border()),
            Button("Next Step",    [this]() { Compute(); },             ButtonOption::Border()),
        }),
        Slider("Initial Cells %", &m_initial_cells_percent),
        Renderer([&]
        {
            return vbox({
                separator(),
                paragraph("Controls:") | ftxui::bold,
                paragraph(" ◆ Press mouse left button over game field to drag the visible area."),
                separator(),
                paragraph("Conway's Game of Life Rules:") | ftxui::bold,
                paragraph(" ◆ Any live cell with fewer than two live neighbours dies, as if by underpopulation."),
                paragraph(" ◆ Any live cell with two or three live neighbours lives on to the next generation."),
                paragraph(" ◆ Any live cell with more than three live neighbours dies, as if by overpopulation."),
                paragraph(" ◆ Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction."),
                vbox() | yflex,
                separator(),
                paragraph(fmt::format("Powered by {} v{} {}", METHANE_PRODUCT_NAME, METHANE_VERSION_STR, METHANE_PRODUCT_URL))
            }) | yflex;
        }) | yflex
    });

    auto canvas = Renderer([this]
    {
        return ftxui::canvas([this](Canvas& canvas)
        {
            UpdateFrameSize(canvas.width(), canvas.height());
            if (m_screen_refresh_enabled)
            {
                Compute();
            }
            Present(canvas);
        }) | flex;
    });

    auto canvas_with_mouse = CatchEvent(canvas, [this](Event e)
    {
        return HandleInputEvent(e);
    });

    static int s_sidebar_width = 35;
    auto main_container = Container::Vertical(
    {
        toolbar | xflex,
        ResizableSplitLeft(sidebar, canvas_with_mouse, &s_sidebar_width) | border | flex
    });

    m_root = Renderer(main_container, [=]
    {
        return vbox({
            text("Methane Console Compute: Game of Life") | ftxui::bold | hcenter,
            main_container->Render() | flex,
        });
    });
}

void ConsoleApp::UpdateFrameSize(int width, int height)
{
    META_FUNCTION_TASK();
    if (!static_cast<bool>(m_frame_rect.size))
    {
        // Set initial frame position in the center of game field
        m_frame_rect.origin.SetX((m_frame_size.GetWidth() - width) / 2);
        m_frame_rect.origin.SetY((m_frame_size.GetHeight() - height) / 2);
    }

    // Update frame size
    m_frame_rect.size.SetWidth(width);
    m_frame_rect.size.SetHeight(height);
}

bool ConsoleApp::HandleInputEvent(ftxui::Event e)
{
    META_FUNCTION_TASK();
    if (!e.is_mouse())
        return false;

    if (e.mouse().button == ftxui::Mouse::Button::Left)
    {
        const data::Point2I mouse_current_pos(e.mouse().x, e.mouse().y);
        if (m_mouse_pressed_pos.has_value())
        {
            const data::Point2I shift = (*m_mouse_pressed_pos - mouse_current_pos) * 2;
            m_frame_rect.origin.SetX(std::max(0, std::min(m_frame_pressed_pos->GetX() + shift.GetX(),
                                                          static_cast<int32_t>(m_frame_size.GetWidth() - m_frame_rect.size.GetWidth() - 1))));
            m_frame_rect.origin.SetY(std::max(0, std::min(m_frame_pressed_pos->GetY() + shift.GetY(),
                                                          static_cast<int32_t>(m_frame_size.GetHeight() - m_frame_rect.size.GetHeight() - 1))));
        }
        else
        {
            m_mouse_pressed_pos = mouse_current_pos;
            m_frame_pressed_pos = m_frame_rect.origin;
        }
    }
    else if (m_mouse_pressed_pos.has_value())
    {
        m_mouse_pressed_pos.reset();
        m_frame_pressed_pos.reset();
    }
    return false;
}

} // namespace Methane::Tutorials
