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

FILE: ConsoleComputeApp.cpp
Tutorial demonstrating "game of life" computing on GPU in console application

******************************************************************************/

#include "ConsoleComputeApp.h"

#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Data/Math.hpp>
#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>
#include <random>

namespace gfx = Methane::Graphics;
namespace data = Methane::Data;

namespace Methane::Tutorials
{

static const rhi::Devices& GetComputeDevices()
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

static Methane::Data::Bytes GetRandomFrameData(std::mt19937& random_engine, const gfx::FrameSize& frame_size, double initial_cells_ratio)
{
    META_FUNCTION_TASK();
    Methane::Data::Bytes frame_data(frame_size.GetPixelsCount(), std::byte());
    const uint32_t cells_count = static_cast<uint32_t>(static_cast<double>(frame_size.GetPixelsCount()) * initial_cells_ratio);
    std::uniform_int_distribution<uint32_t> dist(0U, frame_size.GetPixelsCount() - 1U);
    auto* cell_values = reinterpret_cast<uint8_t*>(frame_data.data());
    for(uint32_t i = 0; i < cells_count; i++)
    {
        uint32_t p = 0U;
        do
        {
            p = dist(random_engine);
        }
        while(cell_values[p]);
        cell_values[p] = 1U;
    }
    return frame_data;
}

ConsoleComputeApp::ConsoleComputeApp()
    : m_random_engine([]() {
        std::random_device r;
        std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
        return std::mt19937(seed);
    }())
{
    InitUserInterface();
    Init();
}

const rhi::Device* ConsoleComputeApp::GetComputeDevice() const
{
    META_FUNCTION_TASK();
    const int compute_device_index = GetComputeDeviceIndex();
    const rhi::Devices& devices = GetComputeDevices();
    return compute_device_index < static_cast<int>(devices.size()) ? &devices[compute_device_index] : nullptr;
}

int ConsoleComputeApp::Run()
{
    const rhi::Device* device_ptr = GetComputeDevice();
    if (!device_ptr)
    {
        std::cerr << "ERROR: No GPU devices are available for computing!";
        return 1;
    }

    return ConsoleApp::Run();
}

std::string_view ConsoleComputeApp::GetGraphicsApiName() const
{
    return magic_enum::enum_name(rhi::System::GetNativeApi());
}

const std::string& ConsoleComputeApp::GetComputeDeviceName() const
{
    META_FUNCTION_TASK();
    static const std::string s_no_adapter = "N/A";
    const rhi::Device* device_ptr = ConsoleComputeApp::GetComputeDevice();
    return device_ptr ? device_ptr->GetAdapterName() : s_no_adapter;
}

const std::vector<std::string>& ConsoleComputeApp::GetComputeDeviceNames() const
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

uint32_t ConsoleComputeApp::GetFramesCountPerSecond() const
{
    return IsScreenRefreshEnabled() ? m_fps_counter.GetFramesPerSecond() : 0U;
}

uint32_t ConsoleComputeApp::GetVisibleCellsCount() const
{
    return m_visible_cells_count;
}

void ConsoleComputeApp::Init()
{
    META_FUNCTION_TASK();
    const rhi::Device* device_ptr = GetComputeDevice();
    m_compute_context = device_ptr->CreateComputeContext(m_parallel_executor, {});
    m_compute_context.SetName("Game of Life");

    m_compute_state = m_compute_context.CreateComputeState({
            m_compute_context.CreateProgram({
            rhi::Program::ShaderSet { { rhi::ShaderType::Compute, { data::ShaderProvider::Get(), { "GameOfLife", "MainCS" } } } },
            rhi::ProgramInputBufferLayouts { },
            rhi::ProgramArgumentAccessors
            {
                { { rhi::ShaderType::All, "m_frame_texture" }, rhi::ProgramArgumentAccessor::Type::Mutable },
            },
        }),
        rhi::ThreadGroupSize(16U, 16U, 1U)
    });
    m_compute_state.GetProgram().SetName("Game of Life Program");
    m_compute_state.SetName("Game of Life Compute State");

    m_compute_cmd_list = m_compute_context.GetComputeCommandKit().GetQueue().CreateComputeCommandList();
    m_compute_cmd_list.SetName("Game of Life Compute");
    m_compute_cmd_list_set = rhi::CommandListSet({ m_compute_cmd_list.GetInterface() });

    rhi::TextureSettings frame_texture_settings = rhi::TextureSettings::ForImage(
        gfx::Dimensions(GetFieldSize()),
        std::nullopt, gfx::PixelFormat::R8Uint, false,
        rhi::ResourceUsageMask{
            rhi::ResourceUsage::ShaderRead,
            rhi::ResourceUsage::ShaderWrite,
            rhi::ResourceUsage::ReadBack
        }
    );
    m_frame_texture = m_compute_context.CreateTexture(frame_texture_settings);
    m_frame_texture.SetName("Game of Life Frame Texture");

    m_compute_bindings = m_compute_state.GetProgram().CreateBindings({
        { { rhi::ShaderType::All, "g_frame_texture"   }, { { m_frame_texture.GetInterface() } } },
    });
    m_compute_bindings.SetName("Game of Life Compute Bindings");

    RandomizeFrameData();

    // Complete bindings and texture initialization
    m_compute_context.CompleteInitialization();
}

void ConsoleComputeApp::Release()
{
    META_FUNCTION_TASK();
    m_compute_context.WaitForGpu(rhi::ContextWaitFor::ComputeComplete);
    m_compute_bindings = {};
    m_frame_texture    = {};
    m_compute_state    = {};
    m_compute_context  = {};
}

void ConsoleComputeApp::Compute()
{
    META_FUNCTION_TASK();
    const data::FrameSize&       field_size        = GetFieldSize();
    const rhi::CommandQueue&     compute_cmd_queue = m_compute_context.GetComputeCommandKit().GetQueue();
    const rhi::ThreadGroupSize&  thread_group_size = m_compute_state.GetSettings().thread_group_size;
    const rhi::ThreadGroupsCount thread_groups_count(data::DivCeil(field_size.GetWidth(), thread_group_size.GetWidth()),
                                                     data::DivCeil(field_size.GetHeight(), thread_group_size.GetHeight()),
                                                     1U);

    META_DEBUG_GROUP_VAR(s_debum_group, "Compute Frame");
    m_compute_cmd_list.ResetWithState(m_compute_state, &s_debum_group);
    m_compute_cmd_list.SetProgramBindings(m_compute_bindings);
    m_compute_cmd_list.Dispatch(thread_groups_count);
    m_compute_cmd_list.Commit();

    compute_cmd_queue.Execute(m_compute_cmd_list_set);
    m_compute_context.WaitForGpu(rhi::ContextWaitFor::ComputeComplete);
    m_frame_data = std::move(m_frame_texture.GetData(compute_cmd_queue));
    m_fps_counter.OnCpuFrameReadyToPresent();
}

void ConsoleComputeApp::Present(ftxui::Canvas& canvas)
{
    META_FUNCTION_TASK();
    const data::FrameSize& field_size = GetFieldSize();
    const data::FrameRect& frame_rect = GetVisibleFrameRect();
    const uint8_t* cells  = m_frame_data.GetDataPtr<uint8_t>();
    m_visible_cells_count = 0U;

    for (uint32_t y = 0; y < frame_rect.size.GetHeight(); y++)
    {
        const uint32_t cell_y = frame_rect.origin.GetY() + y;
        const uint32_t cell_shift = cell_y * field_size.GetWidth();
        for (uint32_t x = 0; x < frame_rect.size.GetWidth(); x++)
        {
            const uint32_t cell_x = frame_rect.origin.GetX() + x;
            if (cells[cell_shift + cell_x])
            {
                canvas.DrawBlockOn(static_cast<int>(x), static_cast<int>(y));
                m_visible_cells_count++;
            }
        }
    }
    m_fps_counter.OnCpuFramePresented();
}

void ConsoleComputeApp::Restart()
{
    META_FUNCTION_TASK();
    std::unique_lock lock(GetScreenRefreshMutex());
    m_compute_context.WaitForGpu(rhi::ContextWaitFor::ComputeComplete);
    RandomizeFrameData();
    m_compute_context.UploadResources();
}

void ConsoleComputeApp::RandomizeFrameData()
{
    META_FUNCTION_TASK();

    // Randomize initial game state
    m_frame_data = rhi::SubResource(GetRandomFrameData(m_random_engine, GetFieldSize(), GetInitialCellsRatio()));

    // Set frame texture data
    m_frame_texture.SetData(m_compute_context.GetComputeCommandKit().GetQueue(), { m_frame_data });
}

} // namespace Methane::Tutorials

int main(int, const char*[])
{
    return Methane::Tutorials::ConsoleComputeApp().Run();
}