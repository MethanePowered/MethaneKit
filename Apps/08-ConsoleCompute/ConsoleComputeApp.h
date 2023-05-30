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

FILE: ConsoleComputeApp.h
Tutorial demonstrating "game of life" computing on GPU in console application

******************************************************************************/

#pragma once

#include "ConsoleApp.h"

#include <Methane/Kit.h>
#include <Methane/Data/FpsCounter.h>

#include <taskflow/taskflow.hpp>
#include <string>
#include <random>

namespace rhi = Methane::Graphics::Rhi;

namespace Methane::Tutorials
{

class ConsoleComputeApp final
    : public ConsoleApp
{
public:
    ConsoleComputeApp();

    const rhi::Device* GetComputeDevice() const;

    // ConsoleApp overrides
    int Run() override;
    std::string_view                GetGraphicsApiName() const override;
    const std::string&              GetComputeDeviceName() const override;
    const std::vector<std::string>& GetComputeDeviceNames() const override;
    uint32_t                        GetFramesCountPerSecond() const override;
    uint32_t                        GetVisibleCellsCount() const override;

protected:
    void Init() override;
    void Release() override;
    void Compute() override;
    void Present(ftxui::Canvas& canvas) override;
    void Restart() override;

private:
    void RandomizeFrameData();

    std::mt19937            m_random_engine;
    tf::Executor            m_parallel_executor;
    rhi::ComputeContext     m_compute_context;
    rhi::ComputeState       m_compute_state;
    rhi::ComputeCommandList m_compute_cmd_list;
    rhi::CommandListSet     m_compute_cmd_list_set;
    rhi::Texture            m_frame_texture;
    rhi::ProgramBindings    m_compute_bindings;
    rhi::SubResource        m_frame_data;
    Data::FpsCounter        m_fps_counter{ 60U };
    uint32_t                m_visible_cells_count{ 0U };
};

} // namespace Methane::Tutorials
