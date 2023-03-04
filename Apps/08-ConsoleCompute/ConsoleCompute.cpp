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

FILE: ConsoleCompute.cpp
Tutorial demonstrating "game of life" computing on GPU in console application

******************************************************************************/

#include <Methane/Kit.h>
#include <Methane/Version.h>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <fmt/format.h>
#include <random>

namespace gfx = Methane::Graphics;
namespace rhi = Methane::Graphics::Rhi;

const rhi::Device* GetComputeDevice()
{
    rhi::System::Get().UpdateGpuDevices();
    const rhi::Devices& devices = rhi::System::Get().GetGpuDevices();
    if (devices.empty())
    {
        return nullptr;
    }
    return &devices[0];
}

ftxui::Component InitializeConsoleInterface(ftxui::ScreenInteractive& screen, const rhi::Device& device,
                                            gfx::FrameSize& frame_size, std::mt19937& random_engine)
{
    auto toolbar = ftxui::Container::Horizontal({
        ftxui::Renderer([&frame_size, &device]
        {
            return ftxui::hbox({
                ftxui::text(fmt::format(" GPU: {} ", device.GetAdapterName())),
                ftxui::separator(),
                ftxui::text(" FPS: XXX "),
                ftxui::separator(),
                ftxui::text(fmt::format(" Field: {} x {} ", frame_size.GetWidth(), frame_size.GetHeight()))
            });
        }) | ftxui::border | ftxui::xflex,
        ftxui::Button(" X ", screen.ExitLoopClosure(), ftxui::ButtonOption::Simple()) | ftxui::align_right
    });

    auto sidebar = ftxui::Renderer([]
    {
        return ftxui::vbox({
            ftxui::vbox() | ftxui::yflex,
            ftxui::paragraph(fmt::format("Powered by {} v{} {}", METHANE_PRODUCT_NAME, METHANE_VERSION_STR, METHANE_PRODUCT_URL))
        }) | ftxui::yflex;
    });

    auto canvas = ftxui::Renderer([&random_engine, &frame_size]
    {
        return ftxui::canvas([&](ftxui::Canvas& c)
        {
            // Temporary drawing of random points on canvas
            frame_size.SetWidth(c.width());
            frame_size.SetHeight(c.height());
            const uint32_t points_count = frame_size.GetPixelsCount() / 100;
            std::uniform_int_distribution<> dist(0, frame_size.GetPixelsCount() - 1);
            for(uint32_t i = 0; i < points_count; i++)
            {
                int rnd = dist(random_engine);
                int x = rnd % c.width();
                int y = rnd / c.width();
                c.DrawBlockOn(x, y);
            }
        }) | ftxui::flex;
    });

    static int sidebar_width = 35;
    auto main_container = ftxui::Container::Vertical(
    {
        toolbar | ftxui::xflex,
        ftxui::ResizableSplitLeft(sidebar, canvas, &sidebar_width) | ftxui::border | ftxui::flex
    });

    return Renderer(main_container, [=]
    {
        return ftxui::vbox({
            ftxui::text("Methane Console Compute: Game of Life") | ftxui::bold | ftxui::hcenter,
            main_container->Render() | ftxui::flex,
        });
    });
}

void RunEventLoop(ftxui::ScreenInteractive& screen, const ftxui::Component& root, uint32_t& time)
{
    std::atomic<bool> refresh_ui_continue = true;
    std::thread refresh_ui([&screen, &time, &refresh_ui_continue]
    {
        while (refresh_ui_continue)
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.05s);
            screen.Post([&] { time++; });
            screen.Post(ftxui::Event::Custom);
        }
    });

    screen.Loop(root);
    refresh_ui_continue = false;
    refresh_ui.join();
}

int main(int, const char*[])
{
    std::random_device r;
    std::seed_seq random_seed{r(), r(), r(), r(), r(), r(), r(), r()};
    std::mt19937 random_engine(random_seed);
    uint32_t time = 0;
    gfx::FrameSize frame_size;

    const rhi::Device* device_ptr = GetComputeDevice();
    if (!device_ptr)
    {
        std::cerr << "ERROR: No GPU device available for computing!";
        return 1;
    }

    ftxui::ScreenInteractive ui_screen = ftxui::ScreenInteractive::Fullscreen();
    ftxui::Component ui_root = InitializeConsoleInterface(ui_screen, *device_ptr, frame_size, random_engine);
    RunEventLoop(ui_screen, ui_root, time);
    return 0;
}