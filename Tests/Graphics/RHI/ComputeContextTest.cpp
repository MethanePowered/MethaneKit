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

FILE: Tests/Graphics/RHI/ColorTest.cpp
Unit-tests of the RHI ComputeContext

******************************************************************************/

#include "Methane/Graphics/RHI/IComputeContext.h"
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/System.h>
#include <Methane/Graphics/RHI/Device.h>

#include <stdexcept>
#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

static Rhi::Device GetTestDevice()
{
    const Rhi::Devices& devices = Rhi::System::Get().UpdateGpuDevices();
    if (devices.empty())
        throw std::logic_error("No RHI devices available");
    return devices[0];
}

TEST_CASE("RHI Compute Context", "[rhi][compute][context]")
{
    const Rhi::ComputeContextSettings compute_context_settings{
        Rhi::ContextOptionMask{ Rhi::ContextOption::TransferWithD3D12DirectQueue }
    };
    const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);

    SECTION("Check initialization")
    {
        CHECK(compute_context.IsInitialized());
        CHECK(compute_context.GetInterfacePtr());
        CHECK(compute_context.GetInterface().GetSettings() == compute_context_settings);
    }
}
