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

#include "Methane/Graphics/RHI/IResource.h"
#include "RhiTestHelpers.hpp"

#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/Program.h>
#include <Methane/Graphics/RHI/ProgramBindings.h>
#include <Methane/Graphics/RHI/Buffer.h>
#include <Methane/Graphics/Null/Program.h>

#include <memory>
#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

TEST_CASE("RHI Program Bindings Functions", "[rhi][program][bindings]")
{
    const Rhi::ComputeContext compute_context = Rhi::ComputeContext(GetTestDevice(), g_parallel_executor, {});
    const Rhi::Program compute_program = [&compute_context]()
        {
            const Rhi::ProgramArgumentAccessor input_a_accessor{ Rhi::ShaderType::Compute, "InputA", Rhi::ProgramArgumentAccessType::Constant };
            const Rhi::ProgramArgumentAccessor input_b_accessor{ Rhi::ShaderType::Compute, "InputB", Rhi::ProgramArgumentAccessType::Constant };
            const Rhi::ProgramArgumentAccessor output_accessor { Rhi::ShaderType::Compute, "Output", Rhi::ProgramArgumentAccessType::Mutable };
            Rhi::Program compute_program = compute_context.CreateProgram(
                Rhi::ProgramSettingsImpl
                {
                    Rhi::ProgramSettingsImpl::ShaderSet
                    {
                        { Rhi::ShaderType::Compute, { Data::ShaderProvider::Get(), { "Compute", "Main" } } }
                    },
                    Rhi::ProgramInputBufferLayouts{ },
                    Rhi::ProgramArgumentAccessors
                    {
                        input_a_accessor,
                        input_b_accessor,
                        output_accessor
                    }
                });
            dynamic_cast<Null::Program&>(compute_program.GetInterface()).InitArgumentBindings({
                { input_a_accessor, { Rhi::ResourceType::Buffer, 1U } },
                { input_b_accessor, { Rhi::ResourceType::Buffer, 1U } },
                { output_accessor,  { Rhi::ResourceType::Buffer, 1U } },
            });
            return compute_program;
        }();

    const Rhi::Buffer bufferA = compute_context.CreateBuffer(Rhi::BufferSettings::ForConstantBuffer(100500));
    const Rhi::Buffer bufferB = compute_context.CreateBuffer(Rhi::BufferSettings::ForConstantBuffer(12345, false, true));
    const Rhi::Buffer bufferC = compute_context.CreateBuffer(Rhi::BufferSettings::ForConstantBuffer(42000, false, true));

    const Rhi::Program::ResourceViewsByArgument compute_resource_views{
        { { Rhi::ShaderType::All, "InputA" }, { { bufferA.GetInterface() } } },
        { { Rhi::ShaderType::All, "InputB" }, { { bufferB.GetInterface() } } },
        { { Rhi::ShaderType::All, "Output" }, { { bufferC.GetInterface() } } },
    };

    SECTION("Compute Program Bindings Construction")
    {
        Rhi::ProgramBindings program_bindings;
        REQUIRE_NOTHROW(program_bindings = compute_program.CreateBindings(compute_resource_views, 2U));
        REQUIRE(program_bindings.IsInitialized());
        CHECK(program_bindings.GetInterfacePtr());
        CHECK(program_bindings.GetArguments().size() == 3U);
        CHECK(program_bindings.GetFrameIndex() == 2U);
    }

    SECTION("Object Destroyed Callback")
    {
        auto program_bindings_ptr = std::make_unique<Rhi::ProgramBindings>(compute_program, compute_resource_views);
        ObjectCallbackTester object_callback_tester(*program_bindings_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        program_bindings_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    const Rhi::ProgramBindings program_bindings = compute_program.CreateBindings(compute_resource_views);

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
        CHECK(compute_program.SetName("My Compute Context"));
        ObjectCallbackTester object_callback_tester(compute_program);
        CHECK_FALSE(compute_program.SetName("My Compute Context"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Can Get Program Binding Arguments")
    {
        Rhi::ProgramArguments program_arguments;
        REQUIRE(program_bindings.GetArguments().size() == 3U);
        REQUIRE_NOTHROW(program_arguments = program_bindings.GetArguments());
        CHECK(program_arguments.count({ Rhi::ShaderType::All, "InputA" }) == 1);
        CHECK(program_arguments.count({ Rhi::ShaderType::All, "InputB" }) == 1);
        CHECK(program_arguments.count({ Rhi::ShaderType::All, "Output" }) == 1);
    }

    SECTION("Can Get Program Argument Binding")
    {
        Rhi::IProgramArgumentBinding* input_a_binding_ptr = nullptr;
        REQUIRE_NOTHROW(input_a_binding_ptr = &program_bindings.Get({ Rhi::ShaderType::All, "InputA" }));
        REQUIRE(input_a_binding_ptr);
        CHECK(input_a_binding_ptr->GetSettings().argument.GetName() == "InputA");
        CHECK(input_a_binding_ptr->GetSettings().resource_count == 1U);
        CHECK(input_a_binding_ptr->GetSettings().resource_type == Rhi::ResourceType::Buffer);
        CHECK(input_a_binding_ptr->GetResourceViews().size() == 1U);
        CHECK(input_a_binding_ptr->GetResourceViews().at(0).GetResourcePtr().get() == bufferA.GetInterfacePtr().get());
    }
}