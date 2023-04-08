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

#include "RhiTestHelpers.hpp"

#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/System.h>
#include <Methane/Graphics/RHI/Device.h>

#include <memory>
#include <stdexcept>
#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
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

    SECTION("Context Construction")
    {
        CHECK(compute_context.IsInitialized());
        CHECK(compute_context.GetInterfacePtr());
        CHECK(compute_context.GetInterface().GetSettings() == compute_context_settings);
        CHECK(compute_context.GetName() == "");
    }

    SECTION("Object Destroyed Callback")
    {
        auto compute_context_ptr = std::make_unique<Rhi::ComputeContext>(GetTestDevice(), g_parallel_executor, compute_context_settings);
        ObjectCallbackTester object_callback_tester(compute_context_ptr->GetInterface());
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        compute_context_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    SECTION("Object Name Setup")
    {
        CHECK(compute_context.SetName("My Compute Context"));
        CHECK(compute_context.GetName() == "My Compute Context");
    }

    SECTION("Object Name Change Callback")
    {
        CHECK_NOTHROW(compute_context.SetName("My Compute Context"));
        ObjectCallbackTester object_callback_tester(compute_context.GetInterface());
        CHECK(compute_context.SetName("Our Compute Context"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Compute Context");
        CHECK(object_callback_tester.GetOldObjectName() == "My Compute Context");
    }

    SECTION("Object Name Set Unchanged")
    {
        CHECK_NOTHROW(compute_context.SetName("My Compute Context"));
        ObjectCallbackTester object_callback_tester(compute_context.GetInterface());
        CHECK_FALSE(compute_context.SetName("My Compute Context"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }
}
