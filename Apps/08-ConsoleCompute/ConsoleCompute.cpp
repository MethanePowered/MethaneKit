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
#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Data/Math.hpp>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <taskflow/taskflow.hpp>
#include <magic_enum.hpp>
#include <fmt/format.h>
#include <random>

namespace gfx = Methane::Graphics;
namespace rhi = Methane::Graphics::Rhi;
namespace data = Methane::Data;

static std::mt19937 g_random_engine = []()
{
    std::random_device r;
    std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
    return std::mt19937(seed);
}();

static uint32_t                g_time = 0;
static gfx::FrameSize          g_frame_size;
static int                     g_compute_device_index = 0;
static ftxui::RadioboxOption   g_compute_device_option = ftxui::RadioboxOption::Simple();
static tf::Executor            g_parallel_executor;
static rhi::ComputeContext     g_compute_context;
static rhi::ComputeState       g_compute_state;
static rhi::ComputeCommandList g_compute_cmd_list;
static rhi::CommandListSet     g_compute_cmd_list_set;
static rhi::Texture            g_frame_texture;
static rhi::ProgramBindings    g_compute_bindings;
static rhi::SubResource        g_frame_data;

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

Methane::Data::Bytes GetRandomFrameData(const gfx::FrameSize& frame_size)
{
    Methane::Data::Bytes frame_data(frame_size.GetPixelsCount(), std::byte());
    const uint32_t points_count = frame_size.GetPixelsCount() / 10;
    std::uniform_int_distribution<> dist(0, frame_size.GetPixelsCount() - 1);
    auto* cell_values = reinterpret_cast<uint8_t*>(frame_data.data());
    for(uint32_t i = 0; i < points_count; i++)
    {
        cell_values[dist(g_random_engine)] = 1U;
    }
    return frame_data;
}

void InitializeFrameTexture()
{
    if (!g_frame_size)
    {
        g_frame_texture = {};
        return;
    }

    rhi::TextureSettings frame_texture_settings = rhi::TextureSettings::ForImage(
        gfx::Dimensions(g_frame_size),
        std::nullopt, gfx::PixelFormat::R8Uint, false,
        rhi::ResourceUsageMask{ rhi::ResourceUsage::ShaderRead, rhi::ResourceUsage::ShaderWrite }
    );
    frame_texture_settings.usage_mask.SetBitOn(rhi::ResourceUsage::ReadBack);
    g_frame_texture = g_compute_context.CreateTexture(frame_texture_settings);
    g_frame_texture.SetName("Game of Life Frame Texture");

    g_compute_bindings = g_compute_state.GetProgram().CreateBindings({
        { { rhi::ShaderType::All, "g_frame_texture"   }, { { g_frame_texture.GetInterface() } } },
    });
    g_compute_bindings.SetName("Game of Life Compute Bindings");

    // Complete bindings initialization
    g_compute_context.CompleteInitialization();

    // Randomize initial game state
    g_frame_data = rhi::SubResource(GetRandomFrameData(g_frame_size));

    // Set frame texture data
    g_frame_texture.SetData({ g_frame_data }, g_compute_context.GetComputeCommandKit().GetQueue());
}

void ReleaseComputeContext()
{
    g_compute_context.WaitForGpu(rhi::ContextWaitFor::ComputeComplete);

    g_compute_bindings = {};
    g_frame_texture    = {};
    g_compute_state    = {};
    g_compute_context  = {};
}

void InitializeComputeContext()
{
    const rhi::Device* device_ptr = GetComputeDevice();
    g_compute_context = device_ptr->CreateComputeContext(g_parallel_executor, {});
    g_compute_context.SetName("Game of Life");

    g_compute_state = g_compute_context.CreateComputeState({
        g_compute_context.CreateProgram({
            rhi::Program::ShaderSet { { rhi::ShaderType::Compute, { data::ShaderProvider::Get(), { "GameOfLife", "MainCS" } } } },
            rhi::ProgramInputBufferLayouts { },
            rhi::ProgramArgumentAccessors
            {
                { { rhi::ShaderType::All, "g_frame_texture" }, rhi::ProgramArgumentAccessor::Type::Mutable },
            },
        }),
        rhi::ThreadGroupSize(16U, 16U, 1U)
    });
    g_compute_state.GetProgram().SetName("Game of Life Program");
    g_compute_state.SetName("Game of Life Compute State");

    g_compute_cmd_list = g_compute_context.GetComputeCommandKit().GetQueue().CreateComputeCommandList();
    g_compute_cmd_list.SetName("Game of Life Compute");

    g_compute_cmd_list_set = rhi::CommandListSet({ g_compute_cmd_list.GetInterface() });

    InitializeFrameTexture();
}

void ComputeFrame()
{
    const rhi::CommandQueue&     compute_cmd_queue = g_compute_context.GetComputeCommandKit().GetQueue();
    const rhi::ThreadGroupSize&  thread_group_size = g_compute_state.GetSettings().thread_group_size;
    const rhi::ThreadGroupsCount thread_groups_count(data::DivCeil(g_frame_size.GetWidth(), thread_group_size.GetWidth()),
                                                     data::DivCeil(g_frame_size.GetHeight(), thread_group_size.GetHeight()),
                                                     1U);

    META_DEBUG_GROUP_VAR(s_debug_group, "Compute Frame");
    g_compute_cmd_list.ResetWithState(g_compute_state, &s_debug_group);
    g_compute_cmd_list.SetProgramBindings(g_compute_bindings);
    g_compute_cmd_list.Dispatch(thread_groups_count);
    g_compute_cmd_list.Commit();

    compute_cmd_queue.Execute(g_compute_cmd_list_set);

    // FIXME: Temporary disable frame data getter for Vulkan and Metal, to fix unreachable code warning/error
    if constexpr (rhi::System::GetNativeApi() != rhi::NativeApi::DirectX)
    {
        g_frame_data = std::move(g_frame_texture.GetData(compute_cmd_queue.GetInterface()));
    }

    g_compute_context.WaitForGpu(rhi::ContextWaitFor::ComputeComplete);
}

void DrawFrame(ftxui::Canvas& canvas)
{
    const uint8_t* cell_values = g_frame_data.GetDataPtr<uint8_t>();
    const rhi::TextureSettings& frame_texture_settings = g_frame_texture.GetSettings();
    for(uint32_t y = 0; y < frame_texture_settings.dimensions.GetHeight(); y++)
        for(uint32_t x = 0; x < frame_texture_settings.dimensions.GetWidth(); x++)
        {
            const uint32_t cell_index = x + y * frame_texture_settings.dimensions.GetWidth();
            if (cell_values[cell_index])
            {
                canvas.DrawBlockOn(static_cast<int>(x), static_cast<int>(y));
            }
        }
}

ftxui::Component InitializeConsoleInterface(ftxui::ScreenInteractive& screen)
{
    using namespace ftxui;
    auto toolbar = Container::Horizontal({
        Renderer([]
        {
            return hbox({
                text(fmt::format(" API: {} ", magic_enum::enum_name(rhi::System::GetNativeApi()))),
                separator(),
                text(fmt::format(" GPU: {} ", GetComputeDevice()->GetAdapterName())),
                separator(),
                text(" FPS: XXX "),
                separator(),
                text(fmt::format(" Field: {} x {} ", g_frame_size.GetWidth(), g_frame_size.GetHeight()))
            });
        }) | border | xflex,
        Button(" X ", screen.ExitLoopClosure(), ButtonOption::Simple()) | align_right
    });

    g_compute_device_option.on_change = []()
    {
        ReleaseComputeContext();
        InitializeComputeContext();
    };

    auto sidebar = Container::Vertical({
        Renderer([&]{ return text("GPU Devices:") | ftxui::bold; }),
        Radiobox(&GetComputeDeviceNames(), &g_compute_device_index, g_compute_device_option),
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

    auto canvas = Renderer([]
    {
        return ftxui::canvas([&](Canvas& canvas)
        {
            // Update frame size
            const gfx::FrameSize frame_size(canvas.width(), canvas.height());
            if (g_frame_size != frame_size)
            {
                g_frame_size = frame_size;
                InitializeFrameTexture();
            }

            // Compute turn in Game of Life and draw on frame
            ComputeFrame();
            //DrawFrame(canvas);
        }) | flex;
    });

    static int s_sidebar_width = 35;
    auto main_container = Container::Vertical(
    {
        toolbar | xflex,
        ResizableSplitLeft(sidebar, canvas, &s_sidebar_width) | border | flex
    });

    return Renderer(main_container, [=]
    {
        return vbox({
            text("Methane Console Compute: Game of Life") | ftxui::bold | hcenter,
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
    const rhi::Device* device_ptr = GetComputeDevice();
    if (!device_ptr)
    {
        std::cerr << "ERROR: No GPU devices are available for computing!";
        return 1;
    }

    ftxui::ScreenInteractive ui_screen = ftxui::ScreenInteractive::Fullscreen();
    ftxui::Component ui_root = InitializeConsoleInterface(ui_screen);
    InitializeComputeContext();
    RunEventLoop(ui_screen, ui_root);
    return 0;
}