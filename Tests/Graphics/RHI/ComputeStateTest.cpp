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
Unit-tests of the RHI ComputeState

******************************************************************************/

#include "RhiTestHelpers.hpp"

#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/ComputeState.h>
#include <Methane/Graphics/RHI/Program.h>
#include <Methane/Graphics/RHI/Shader.h>
#include <Methane/Graphics/RHI/ObjectRegistry.h>

#include <memory>
#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

TEST_CASE("RHI Compute State Functions", "[rhi][compute][state]")
{
    const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, {});
    const Rhi::ComputeStateSettingsImpl compute_state_settings{
        compute_context.CreateProgram({
            { { Rhi::ShaderType::Compute, { Data::ShaderProvider::Get(), { "Shader", "Main" } } } },
        }),
        Rhi::ThreadGroupSize(16, 16, 1)
    };

    SECTION("Compute State Construction")
    {
        Rhi::ComputeState compute_state;
        REQUIRE_NOTHROW(compute_state = compute_context.CreateComputeState(compute_state_settings));
        REQUIRE(compute_state.IsInitialized());
        CHECK(compute_state.GetInterfacePtr());
        CHECK(compute_state.GetSettings() == Rhi::ComputeStateSettingsImpl::Convert(compute_state_settings));
    }

    SECTION("Object Destroyed Callback")
    {
        auto compute_state_ptr = std::make_unique<Rhi::ComputeState>(compute_context, compute_state_settings);
        ObjectCallbackTester object_callback_tester(*compute_state_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        compute_state_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    const Rhi::ComputeState compute_state = compute_context.CreateComputeState(compute_state_settings);

    SECTION("Object Name Setup")
    {
        CHECK(compute_state.SetName("My Compute State"));
        CHECK(compute_state.GetName() == "My Compute State");
    }

    SECTION("Object Name Change Callback")
    {
        CHECK(compute_state.SetName("My Compute State"));
        ObjectCallbackTester object_callback_tester(compute_state);
        CHECK(compute_state.SetName("Our Compute State"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Compute State");
        CHECK(object_callback_tester.GetOldObjectName() == "My Compute State");
    }

    SECTION("Object Name Set Unchanged")
    {
        CHECK(compute_state.SetName("My Compute State"));
        ObjectCallbackTester object_callback_tester(compute_state);
        CHECK_FALSE(compute_state.SetName("My Compute State"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Add to Objects Registry")
    {
        compute_state.SetName("Compute State");
        Rhi::ObjectRegistry registry = compute_context.GetObjectRegistry();
        registry.AddGraphicsObject(compute_state);
        const auto registered_compute_state = registry.GetGraphicsObject<Rhi::ComputeState>("Compute State");
        REQUIRE(registered_compute_state.IsInitialized());
        CHECK(&registered_compute_state.GetInterface() == &compute_state.GetInterface());
    }

    SECTION("Reset with Settings Impl")
    {
        const Rhi::Program new_compute_program = compute_context.CreateProgram({
            { { Rhi::ShaderType::Compute, { Data::ShaderProvider::Get(), { "Compute", "New" } } } },
        });
        REQUIRE_NOTHROW(compute_state.Reset(Rhi::ComputeStateSettingsImpl{ new_compute_program, Rhi::ThreadGroupSize(32, 32, 1) }));
        REQUIRE(compute_state.GetProgram().GetInterfacePtr().get() == new_compute_program.GetInterfacePtr().get());
        REQUIRE(compute_state.GetSettings().thread_group_size == Rhi::ThreadGroupSize(32, 32, 1));
    }

    SECTION("Reset with Settings")
    {
        const Rhi::Program new_compute_program = compute_context.CreateProgram({
            { { Rhi::ShaderType::Compute, { Data::ShaderProvider::Get(), { "Compute", "New" } } } },
        });
        REQUIRE_NOTHROW(compute_state.Reset(Rhi::ComputeStateSettings{ new_compute_program.GetInterfacePtr(), Rhi::ThreadGroupSize(32, 32, 1) }));
        REQUIRE(compute_state.GetProgram().GetInterfacePtr().get() == new_compute_program.GetInterfacePtr().get());
        REQUIRE(compute_state.GetSettings().thread_group_size == Rhi::ThreadGroupSize(32, 32, 1));
    }
}
