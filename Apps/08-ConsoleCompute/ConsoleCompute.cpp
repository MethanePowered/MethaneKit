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
#include <magic_enum.hpp>
#include <fmt/format.h>
#include <random>

namespace gfx = Methane::Graphics;
namespace rhi = Methane::Graphics::Rhi;

static uint32_t g_time = 0;
static int g_compute_device_index = 0;

const rhi::Devices& GetComputeDevices()
{
    static const rhi::Devices& s_compute_devices = []()
    {
        rhi::System::Get().UpdateGpuDevices(rhi::DeviceCaps{
            rhi::DeviceFeatureMask{},
            0U, // render_queues_count
            1U, // transfer_queues_count
            1U  // compute_queues_count
        });
        return rhi::System::Get().GetGpuDevices();
    }();
    return s_compute_devices;
}

const std::vector<std::string>& GetComputeDeviceNames()
{
    static const std::vector<std::string> s_compute_device_names = []()
    {
        std::vector<std::string> device_names;
        for(const rhi::Device& device : GetComputeDevices())
        {
            device_names.emplace_back(device.GetAdapterName());
        }
        return device_names;
    }();
    return s_compute_device_names;
}

const rhi::Device* GetComputeDevice()
{
    const rhi::Devices& devices = GetComputeDevices();
    return g_compute_device_index < static_cast<int>(devices.size()) ? &devices[g_compute_device_index] : nullptr;
}

ftxui::Component InitializeConsoleInterface(ftxui::ScreenInteractive& screen, gfx::FrameSize& frame_size, std::mt19937& random_engine)
{
    using namespace ftxui;
    auto toolbar = Container::Horizontal({
        Renderer([&frame_size]
        {
            return hbox({
                text(fmt::format(" API: {} ", magic_enum::enum_name(rhi::System::GetNativeApi()))),
                separator(),
                text(fmt::format(" GPU: {} ", GetComputeDevice()->GetAdapterName())),
                separator(),
                text(" FPS: XXX "),
                separator(),
                text(fmt::format(" Field: {} x {} ", frame_size.GetWidth(), frame_size.GetHeight()))
            });
        }) | border | xflex,
        Button(" X ", screen.ExitLoopClosure(), ButtonOption::Simple()) | align_right
    });

    auto sidebar = Container::Vertical({
        Renderer([&]{ return text("GPU Devices:") | bold; }),
        Radiobox(&GetComputeDeviceNames(), &g_compute_device_index),
        Renderer([&]
        {
            return vbox({
                separator(),
                vbox() | yflex,
                separator(),
                paragraph(fmt::format("Powered by {} v{} {}", METHANE_PRODUCT_NAME, METHANE_VERSION_STR, METHANE_PRODUCT_URL))
            }) | yflex;
        }) | yflex
    });

    auto canvas = Renderer([&random_engine, &frame_size]
    {
        return ftxui::canvas([&](Canvas& c)
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
        }) | flex;
    });

    static int sidebar_width = 35;
    auto main_container = Container::Vertical(
    {
        toolbar | xflex,
        ResizableSplitLeft(sidebar, canvas, &sidebar_width) | border | flex
    });

    return Renderer(main_container, [=]
    {
        return vbox({
            text("Methane Console Compute: Game of Life") | bold | hcenter,
            main_container->Render() | flex,
        });
    });
}

void RunEventLoop(ftxui::ScreenInteractive& screen, const ftxui::Component& root)
{
    std::atomic<bool> refresh_ui_continue = true;
    std::thread refresh_ui([&screen, &refresh_ui_continue]
    {
        while (refresh_ui_continue)
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.05s);
            screen.Post([&] { g_time++; });
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
    gfx::FrameSize frame_size;

    const rhi::Device* device_ptr = GetComputeDevice();
    if (!device_ptr)
    {
        std::cerr << "ERROR: No GPU devices are available for computing!";
        return 1;
    }

    ftxui::ScreenInteractive ui_screen = ftxui::ScreenInteractive::Fullscreen();
    ftxui::Component ui_root = InitializeConsoleInterface(ui_screen, frame_size, random_engine);
    RunEventLoop(ui_screen, ui_root);
    return 0;
}