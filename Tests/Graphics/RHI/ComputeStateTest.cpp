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

FILE: Tests/Graphics/RHI/ComputeStateTest.cpp
Unit-tests of the RHI ComputeContext

******************************************************************************/

#include "RhiTestHelpers.hpp"

#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/ComputeState.h>
#include <Methane/Graphics/RHI/Program.h>
#include <Methane/Graphics/RHI/Shader.h>

#include <memory>
#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

TEST_CASE("RHI Compute State Functions", "[rhi][compute][state]")
{
    const Rhi::ComputeContext compute_context = Rhi::ComputeContext(GetTestDevice(), g_parallel_executor, {});
    const Rhi::ComputeStateSettingsImpl& compute_state_settings{
        compute_context.CreateProgram({
            { { Rhi::ShaderType::Compute, { Data::ShaderProvider::Get(), { "Shader", "Main" } } } },
        }),
        Rhi::ThreadGroupSize(16, 16, 1)
    };

    SECTION("Context Construction")
    {
        Rhi::ComputeState compute_state;
        REQUIRE_NOTHROW(compute_state = compute_context.CreateComputeState(compute_state_settings));
        REQUIRE(compute_state.IsInitialized());
        CHECK(compute_state.GetInterfacePtr());
        CHECK(compute_state.GetSettings().thread_group_size == compute_state_settings.thread_group_size);
        CHECK(compute_state.GetProgram().GetInterfacePtr().get() == compute_state_settings.program.GetInterfacePtr().get());
    }

    SECTION("Object Destroyed Callback")
    {
        auto compute_state_ptr = std::make_unique<Rhi::ComputeState>(compute_context, compute_state_settings);
        ObjectCallbackTester object_callback_tester(*compute_state_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        compute_state_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    SECTION("Object Name Setup")
    {
        const Rhi::ComputeState compute_state = compute_context.CreateComputeState(compute_state_settings);
        CHECK(compute_context.SetName("My Compute Context"));
        CHECK(compute_context.GetName() == "My Compute Context");
    }

    SECTION("Object Name Change Callback")
    {
        const Rhi::ComputeState compute_state = compute_context.CreateComputeState(compute_state_settings);
        CHECK(compute_context.SetName("My Compute Context"));
        ObjectCallbackTester object_callback_tester(compute_context);
        CHECK(compute_context.SetName("Our Compute Context"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Compute Context");
        CHECK(object_callback_tester.GetOldObjectName() == "My Compute Context");
    }

    SECTION("Object Name Set Unchanged")
    {
        const Rhi::ComputeState compute_state = compute_context.CreateComputeState(compute_state_settings);
        CHECK(compute_context.SetName("My Compute Context"));
        ObjectCallbackTester object_callback_tester(compute_context);
        CHECK_FALSE(compute_context.SetName("My Compute Context"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Reset with Settings Impl")
    {
        const Rhi::Program new_compute_program = compute_context.CreateProgram({
            { { Rhi::ShaderType::Compute, { Data::ShaderProvider::Get(), { "Compute", "New" } } } },
        });
        const Rhi::ComputeState compute_state = compute_context.CreateComputeState(compute_state_settings);
        REQUIRE_NOTHROW(compute_state.Reset(Rhi::ComputeStateSettingsImpl{ new_compute_program, Rhi::ThreadGroupSize(32, 32, 1) }));
        REQUIRE(compute_state.GetProgram().GetInterfacePtr().get() == new_compute_program.GetInterfacePtr().get());
        REQUIRE(compute_state.GetSettings().thread_group_size == Rhi::ThreadGroupSize(32, 32, 1));
    }

    SECTION("Reset with Settings")
    {
        const Rhi::Program new_compute_program = compute_context.CreateProgram({
            { { Rhi::ShaderType::Compute, { Data::ShaderProvider::Get(), { "Compute", "New" } } } },
        });
        const Rhi::ComputeState compute_state = compute_context.CreateComputeState(compute_state_settings);
        REQUIRE_NOTHROW(compute_state.Reset(Rhi::ComputeStateSettings{ new_compute_program.GetInterfacePtr(), Rhi::ThreadGroupSize(32, 32, 1) }));
        REQUIRE(compute_state.GetProgram().GetInterfacePtr().get() == new_compute_program.GetInterfacePtr().get());
        REQUIRE(compute_state.GetSettings().thread_group_size == Rhi::ThreadGroupSize(32, 32, 1));
    }
}
