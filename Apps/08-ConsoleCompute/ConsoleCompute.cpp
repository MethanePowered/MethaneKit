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
#include <Methane/Data/FpsCounter.h>
#include <Methane/Data/Math.hpp>
#include <Methane/Instrumentation.h>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <taskflow/taskflow.hpp>
#include <magic_enum.hpp>
#include <fmt/format.h>
#include <random>
#include <mutex>

namespace gfx = Methane::Graphics;
namespace rhi = Methane::Graphics::Rhi;
namespace data = Methane::Data;

static std::mt19937 g_random_engine = []()
{
    std::random_device r;
    std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
    return std::mt19937(seed);
}();

static std::mutex                   g_screen_refresh_mutex;
static std::atomic<bool>            g_screen_refresh_enabled(true);
static const gfx::FrameSize         g_field_size(2048U, 2048U);
static gfx::FrameRect               g_frame_rect;
static uint32_t                     g_cells_count = 0U;
static int                          g_compute_device_index = 0;
static ftxui::RadioboxOption        g_compute_device_option = ftxui::RadioboxOption::Simple();
static tf::Executor                 g_parallel_executor;
static rhi::ComputeContext          g_compute_context;
static rhi::ComputeState            g_compute_state;
static rhi::ComputeCommandList      g_compute_cmd_list;
static rhi::CommandListSet          g_compute_cmd_list_set;
static rhi::Texture                 g_frame_texture;
static rhi::ProgramBindings         g_compute_bindings;
static rhi::SubResource             g_frame_data;
static data::FpsCounter             g_fps_counter(60U);
static std::optional<data::Point2I> g_mouse_pressed_pos;
static std::optional<data::Point2I> g_frame_pressed_pos;
static bool                         g_30fps_limit_enabled = true;
static bool                         g_game_paused = false;
static int                          g_initial_cells_percent = 50U;

const rhi::Devices& GetComputeDevices()
{
    META_FUNCTION_TASK();
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
    META_FUNCTION_TASK();
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
    META_FUNCTION_TASK();
    const rhi::Devices& devices = GetComputeDevices();
    return g_compute_device_index < static_cast<int>(devices.size()) ? &devices[g_compute_device_index] : nullptr;
}

Methane::Data::Bytes GetRandomFrameData(const gfx::FrameSize& frame_size)
{
    META_FUNCTION_TASK();
    Methane::Data::Bytes frame_data(frame_size.GetPixelsCount(), std::byte());
    const uint32_t cells_count = static_cast<uint32_t>(static_cast<double>(frame_size.GetPixelsCount()) * g_initial_cells_percent / 100.0);
    std::uniform_int_distribution<uint32_t> dist(0U, frame_size.GetPixelsCount() - 1U);
    auto* cell_values = reinterpret_cast<uint8_t*>(frame_data.data());
    for(uint32_t i = 0; i < cells_count; i++)
    {
        uint32_t p = 0U;
        do
        {
            p = dist(g_random_engine);
        }
        while(cell_values[p]);
        cell_values[p] = 1U;
    }
    return frame_data;
}

void RandomizeFrameData()
{
    META_FUNCTION_TASK();

    // Randomize initial game state
    g_frame_data = rhi::SubResource(GetRandomFrameData(g_field_size));

    // Set frame texture data
    g_frame_texture.SetData(g_compute_context.GetComputeCommandKit().GetQueue(), { g_frame_data });
}

void InitializeFrameTexture()
{
    META_FUNCTION_TASK();
    rhi::TextureSettings frame_texture_settings = rhi::TextureSettings::ForImage(
        gfx::Dimensions(g_field_size),
        std::nullopt, gfx::PixelFormat::R8Uint, false,
        rhi::ResourceUsageMask{
            rhi::ResourceUsage::ShaderRead,
            rhi::ResourceUsage::ShaderWrite,
            rhi::ResourceUsage::ReadBack
        }
    );
    g_frame_texture = g_compute_context.CreateTexture(frame_texture_settings);
    g_frame_texture.SetName("Game of Life Frame Texture");

    g_compute_bindings = g_compute_state.GetProgram().CreateBindings({
        { { rhi::ShaderType::All, "g_frame_texture"   }, { { g_frame_texture.GetInterface() } } },
    });
    g_compute_bindings.SetName("Game of Life Compute Bindings");

    RandomizeFrameData();

    // Complete bindings and texture initialization
    g_compute_context.CompleteInitialization();
}

void InitializeComputeContext()
{
    META_FUNCTION_TASK();
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

void ReleaseComputeContext()
{
    META_FUNCTION_TASK();
    g_compute_context.WaitForGpu(rhi::ContextWaitFor::ComputeComplete);

    g_compute_bindings = {};
    g_frame_texture    = {};
    g_compute_state    = {};
    g_compute_context  = {};
}

void ComputeIteration()
{
    META_FUNCTION_TASK();
    const rhi::CommandQueue&     compute_cmd_queue = g_compute_context.GetComputeCommandKit().GetQueue();
    const rhi::ThreadGroupSize&  thread_group_size = g_compute_state.GetSettings().thread_group_size;
    const rhi::ThreadGroupsCount thread_groups_count(data::DivCeil(g_field_size.GetWidth(), thread_group_size.GetWidth()),
                                                     data::DivCeil(g_field_size.GetHeight(), thread_group_size.GetHeight()),
                                                     1U);

    META_DEBUG_GROUP_VAR(s_debug_group, "Compute Frame");
    g_compute_cmd_list.ResetWithState(g_compute_state, &s_debug_group);
    g_compute_cmd_list.SetProgramBindings(g_compute_bindings);
    g_compute_cmd_list.Dispatch(thread_groups_count);
    g_compute_cmd_list.Commit();

    compute_cmd_queue.Execute(g_compute_cmd_list_set);
    g_compute_context.WaitForGpu(rhi::ContextWaitFor::ComputeComplete);
    g_frame_data = std::move(g_frame_texture.GetData(compute_cmd_queue.GetInterface()));
    g_fps_counter.OnCpuFrameReadyToPresent();
}

void RestartSimulation()
{
    META_FUNCTION_TASK();
    std::unique_lock lock(g_screen_refresh_mutex);
    g_compute_context.WaitForGpu(rhi::ContextWaitFor::ComputeComplete);
    RandomizeFrameData();
    g_compute_context.UploadResources();
}

void PlayPauseToggle()
{
    g_game_paused = !g_game_paused;
    g_screen_refresh_enabled = !g_game_paused;
}

void PresentFrame(ftxui::Canvas& canvas)
{
    META_FUNCTION_TASK();
    const uint8_t* cells  = g_frame_data.GetDataPtr<uint8_t>();
    g_cells_count = 0U;
    for (uint32_t y = 0; y < g_frame_rect.size.GetHeight(); y++)
        for (uint32_t x = 0; x < g_frame_rect.size.GetWidth(); x++)
        {
            uint32_t cell_x = g_frame_rect.origin.GetX() + x;
            uint32_t cell_y = g_frame_rect.origin.GetY() + y;
            if (cells[cell_x + cell_y * g_field_size.GetWidth()])
            {
                canvas.DrawBlockOn(static_cast<int>(x), static_cast<int>(y));
                g_cells_count++;
            }
        }
    g_fps_counter.OnCpuFramePresented();
}

ftxui::Component InitializeConsoleInterface(ftxui::ScreenInteractive& screen)
{
    META_FUNCTION_TASK();
    using namespace ftxui;
    auto toolbar = Container::Horizontal({
        Renderer([]
        {
            return hbox({
                text(fmt::format(" API: {} ", magic_enum::enum_name(rhi::System::GetNativeApi()))),
                separator(),
                text(fmt::format(" GPU: {} ", GetComputeDevice()->GetAdapterName())),
                separator(),
                text(fmt::format(" FPS: {} ", g_game_paused ? 0 : g_fps_counter.GetFramesPerSecond())),
                separator(),
                text(fmt::format(" Field: {} x {} ", g_field_size.GetWidth(), g_field_size.GetHeight())),
                separator(),
                text(fmt::format(" Visible {} ", static_cast<std::string>(g_frame_rect) )),
                separator(),
                text(fmt::format(" Visible Cells {} ", g_cells_count ))
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
        Renderer([&] { return separator(); }),
        Checkbox("30 FPS limit", &g_30fps_limit_enabled),
        Container::Horizontal({
            Button("Restart",      []() { RestartSimulation(); }, ButtonOption::Border()),
            Button("Play | Pause", []() { PlayPauseToggle(); },   ButtonOption::Border()),
            Button("Next Step",    []() { ComputeIteration(); },  ButtonOption::Border()),
        }),
        Slider("Initial Cells %", &g_initial_cells_percent),
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

    auto canvas = Renderer([]
    {
        return ftxui::canvas([&](Canvas& canvas)
        {
            if (!static_cast<bool>(g_frame_rect.size))
            {
                // Set initial frame position in the center of game field
                g_frame_rect.origin.SetX((g_field_size.GetWidth() - canvas.width()) / 2);
                g_frame_rect.origin.SetY((g_field_size.GetHeight() - canvas.height()) / 2);
            }
            // Update frame size
            g_frame_rect.size.SetWidth(canvas.width());
            g_frame_rect.size.SetHeight(canvas.height());

            // Compute turn in Game of Life and draw on frame
            if (!g_game_paused)
            {
                ComputeIteration();
            }
            PresentFrame(canvas);
        }) | flex;
    });

    auto canvas_with_mouse = CatchEvent(canvas, [](Event e)
    {
        META_SCOPE_TASK("Mouse Handling");
        if (!e.is_mouse())
            return false;

        if (e.mouse().button == Mouse::Button::Left)
        {
            const data::Point2I mouse_current_pos(e.mouse().x, e.mouse().y);
            if (g_mouse_pressed_pos.has_value())
            {
                const data::Point2I shift = (*g_mouse_pressed_pos - mouse_current_pos) * 2;
                g_frame_rect.origin.SetX(std::max(0, std::min(g_frame_pressed_pos->GetX() + shift.GetX(),
                                                              static_cast<int32_t>(g_field_size.GetWidth() - g_frame_rect.size.GetWidth() - 1))));
                g_frame_rect.origin.SetY(std::max(0, std::min(g_frame_pressed_pos->GetY() + shift.GetY(),
                                                              static_cast<int32_t>(g_field_size.GetHeight() - g_frame_rect.size.GetHeight() - 1))));
            }
            else
            {
                g_mouse_pressed_pos = mouse_current_pos;
                g_frame_pressed_pos = g_frame_rect.origin;
            }
        }
        else if (g_mouse_pressed_pos.has_value())
        {
            g_mouse_pressed_pos.reset();
            g_frame_pressed_pos.reset();
        }
        return false;
    });

    static int s_sidebar_width = 35;
    auto main_container = Container::Vertical(
    {
        toolbar | xflex,
        ResizableSplitLeft(sidebar, canvas_with_mouse, &s_sidebar_width) | border | flex
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
    META_FUNCTION_TASK();
    std::atomic<bool> refresh_ui_continue = true;
    std::thread refresh_ui([&screen, &refresh_ui_continue]
    {
        uint32_t time = 0;
        std::condition_variable_any update_condition_var;
        while (refresh_ui_continue)
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(g_30fps_limit_enabled ? 32ms : 1ms);
            std::unique_lock lock(g_screen_refresh_mutex);
            update_condition_var.wait_for(lock, 1s, [] { return g_screen_refresh_enabled.load(); });
            screen.Post([&time] { time++; });
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