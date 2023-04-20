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

FILE: Tests/Graphics/RHI/ProgramTest.cpp
Unit-tests of the RHI Program

******************************************************************************/

#include "RhiTestHelpers.hpp"

#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/Program.h>
#include <Methane/Graphics/RHI/ProgramBindings.h>
#include <Methane/Graphics/RHI/Shader.h>

#include <memory>
#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

void CheckShaderSettings(const Methane::Ptrs<Rhi::IShader>& shader_ptrs, const Rhi::ProgramSettingsImpl::ShaderSet& shader_settings)
{
    for(const auto& shader_ptr : shader_ptrs)
    {
        REQUIRE(shader_ptr);
        const auto shader_it = shader_settings.find(shader_ptr->GetType());
        REQUIRE(shader_it != shader_settings.end());
        CHECK(shader_ptr->GetSettings() == shader_it->second);
    }
}

const Rhi::ComputeContext compute_context = Rhi::ComputeContext(GetTestDevice(), g_parallel_executor, {});
const Rhi::ProgramSettingsImpl compute_program_settings{
    { { Rhi::ShaderType::Compute, { Data::ShaderProvider::Get(), { "Compute", "Main" } } } },
};

TEST_CASE("RHI Program Functions", "[rhi][program]")
{
    SECTION("Compute Program Construction")
    {
        Rhi::Program compute_program;
        REQUIRE_NOTHROW(compute_program = compute_context.CreateProgram(compute_program_settings));
        REQUIRE(compute_program.IsInitialized());
        CHECK(compute_program.GetInterfacePtr());
        CheckShaderSettings(compute_program.GetSettings().shaders, compute_program_settings.shader_set);
    }

    SECTION("Object Destroyed Callback")
    {
        auto program_ptr = std::make_unique<Rhi::Program>(compute_context, compute_program_settings);
        ObjectCallbackTester object_callback_tester(*program_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        program_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    const Rhi::Program compute_program = compute_context.CreateProgram(compute_program_settings);

    SECTION("Object Name Setup")
    {
        CHECK(compute_program.SetName("My Program"));
        CHECK(compute_program.GetName() == "My Program");
    }

    SECTION("Object Name Change Callback")
    {
        CHECK(compute_program.SetName("My Program"));
        ObjectCallbackTester object_callback_tester(compute_program);
        CHECK(compute_program.SetName("Our Program"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Program");
        CHECK(object_callback_tester.GetOldObjectName() == "My Program");
    }

    SECTION("Object Name Set Unchanged")
    {
        CHECK(compute_program.SetName("My Program"));
        ObjectCallbackTester object_callback_tester(compute_program);
        CHECK_FALSE(compute_program.SetName("My Program"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Can Get Shader Types")
    {
        CHECK(compute_program.GetShaderTypes() == Rhi::ShaderTypes{ Rhi::ShaderType::Compute });
    }

    SECTION("Can Get Existing Shader By Type")
    {
        Rhi::Shader compute_shader;
        REQUIRE_NOTHROW(compute_shader = compute_program.GetShader(Rhi::ShaderType::Compute));
        CHECK(compute_shader.GetType() == Rhi::ShaderType::Compute);
        CHECK(compute_shader.GetSettings() == compute_program_settings.shader_set.at(Rhi::ShaderType::Compute));
    }
}

TEST_CASE("RHI Program Factory", "[rhi][program][factory]")
{
    const Rhi::Program compute_program = compute_context.CreateProgram(Rhi::ProgramSettingsImpl{
        { { Rhi::ShaderType::Compute, { Data::ShaderProvider::Get(), { "Compute", "Main" } } } },
    });

    SECTION("Can Create Program Bindings")
    {
        Rhi::ProgramBindings program_bindings;
        CHECK(compute_program.GetBindingsCount() == 0);
        REQUIRE_NOTHROW(program_bindings = compute_program.CreateBindings({ }));
        CHECK(compute_program.GetBindingsCount() == 1);
        REQUIRE(program_bindings.IsInitialized());
        CHECK(program_bindings.GetInterfacePtr());
    }
}