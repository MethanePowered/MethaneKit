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

FILE: Tests/Graphics/RHI/ShaderTest.cpp
Unit-tests of the RHI Shader

******************************************************************************/

#include "RhiTestHelpers.hpp"

#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/Shader.h>

#include <memory>
#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

TEST_CASE("RHI Shader Functions", "[rhi][compute][state]")
{
    const Rhi::ComputeContext compute_context = Rhi::ComputeContext(GetTestDevice(), g_parallel_executor, {});
    const Rhi::ShaderSettings shader_settings{
        Data::ShaderProvider::Get(),
        Rhi::ShaderEntryFunction{ "Shader", "Main" },
        Rhi::ShaderMacroDefinitions{ { "MACRO_FOO", "1" }, { "MACRO_BAR", "2" } }
    };

    SECTION("Compute Shader Construction from Compute Context")
    {
        Rhi::Shader compute_shader;
        REQUIRE_NOTHROW(compute_shader = compute_context.CreateShader(Rhi::ShaderType::Compute, shader_settings));
        REQUIRE(compute_shader.IsInitialized());
        CHECK(compute_shader.GetInterfacePtr());
        CHECK(compute_shader.GetType() == Rhi::ShaderType::Compute);
        CHECK(compute_shader.GetSettings() == shader_settings);
    }

    SECTION("Macro-definitions to string")
    {
        CHECK(Rhi::ShaderMacroDefinition::ToString(shader_settings.compile_definitions, "; ") ==
              "MACRO_FOO=1; MACRO_BAR=2");
    }
}
