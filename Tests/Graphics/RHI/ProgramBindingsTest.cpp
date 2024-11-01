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

FILE: Tests/Graphics/RHI/ProgramBindingsTest.cpp
Unit-tests of the RHI Program Bindings

******************************************************************************/

#include "RhiTestHelpers.hpp"

#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/Program.h>
#include <Methane/Graphics/RHI/ProgramBindings.h>
#include <Methane/Graphics/RHI/Buffer.h>
#include <Methane/Graphics/RHI/Texture.h>
#include <Methane/Graphics/RHI/Sampler.h>
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
        const Rhi::ProgramArgumentAccessor in_buffer_accessor {
            Rhi::ShaderType::Compute, "InBuffer",
            Rhi::ProgramArgumentAccessType::Constant,
            Rhi::ProgramArgumentValueType::RootConstantBuffer
        };
        const Rhi::ProgramArgumentAccessor in_value_accessor {
            Rhi::ShaderType::Compute, "InValue",
            Rhi::ProgramArgumentAccessType::Mutable,
            Rhi::ProgramArgumentValueType::RootConstantValue
        };
        const Rhi::ProgramArgumentAccessor in_texture_accessor{
            Rhi::ShaderType::Compute, "InTexture",
            Rhi::ProgramArgumentAccessType::Mutable,
            Rhi::ProgramArgumentValueType::ResourceView
        };
        const Rhi::ProgramArgumentAccessor in_sampler_accessor{
            Rhi::ShaderType::Compute, "InSampler",
            Rhi::ProgramArgumentAccessType::Constant,
            Rhi::ProgramArgumentValueType::ResourceView
        };
        const Rhi::ProgramArgumentAccessor out_buffer_accessor {
            Rhi::ShaderType::Compute, "OutBuffer",
            Rhi::ProgramArgumentAccessType::Mutable,
            Rhi::ProgramArgumentValueType::ResourceView
        };

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
                    in_buffer_accessor,
                    in_value_accessor,
                    in_texture_accessor,
                    in_sampler_accessor,
                    out_buffer_accessor
                }
            });
        dynamic_cast<Null::Program&>(compute_program.GetInterface()).SetArgumentBindings({
            { in_buffer_accessor,  { Rhi::ResourceType::Buffer,  1U, 4U } },
            { in_value_accessor,   { Rhi::ResourceType::Buffer,  1U, 4U } },
            { in_texture_accessor, { Rhi::ResourceType::Texture, 1U, 0U } },
            { in_sampler_accessor, { Rhi::ResourceType::Sampler, 1U, 0U } },
            { out_buffer_accessor, { Rhi::ResourceType::Buffer,  1U, 0U } },
        });
        return compute_program;
    }();

    const Rhi::Texture texture1 = [&compute_context]()
    {
        Rhi::Texture texture = compute_context.CreateTexture(Rhi::TextureSettings::ForImage(Dimensions(640, 480), {}, PixelFormat::RGBA8, false));
        CHECK(texture.SetName("T1"));
        return texture;
    }();

    const Rhi::Texture texture2 = [&compute_context]()
    {
        Rhi::Texture texture = compute_context.CreateTexture(Rhi::TextureSettings::ForImage(Dimensions(320, 240), {}, PixelFormat::R8Unorm, false));
        CHECK(texture.SetName("T2"));
        return texture;
    }();

    const Rhi::Sampler sampler = [&compute_context]()
    {
        const Rhi::Sampler sampler = compute_context.CreateSampler({
            rhi::SamplerFilter  { rhi::SamplerFilter::MinMag::Linear },
            rhi::SamplerAddress { rhi::SamplerAddress::Mode::ClampToEdge }
        });
        CHECK(sampler.SetName("S"));
        return sampler;
    }();

    const Rhi::Buffer buffer1 = [&compute_context]()
    {
        const Rhi::Buffer buffer = compute_context.CreateBuffer(Rhi::BufferSettings::ForConstantBuffer(42000, false, true));
        CHECK(buffer.SetName("B1"));
        return buffer;
    }();

    const Rhi::Buffer buffer2 = [&compute_context]()
    {
        const Rhi::Buffer buffer = compute_context.CreateBuffer(Rhi::BufferSettings::ForConstantBuffer(64000, false, true));
        CHECK(buffer.SetName("B2"));
        return buffer;
    }();

    const Rhi::ProgramBindingValueByArgument compute_resource_views{
        { { Rhi::ShaderType::Compute, "InTexture" }, texture1.GetResourceView() },
        { { Rhi::ShaderType::Compute, "InSampler" }, sampler.GetResourceView() },
        { { Rhi::ShaderType::Compute, "OutBuffer" }, buffer1.GetResourceView() },
    };

    SECTION("Create Compute Program Bindings")
    {
        Rhi::ProgramBindings program_bindings;
        REQUIRE_NOTHROW(program_bindings = compute_program.CreateBindings(compute_resource_views, 2U));
        REQUIRE(program_bindings.IsInitialized());
        CHECK(program_bindings.GetInterfacePtr());
        CHECK(program_bindings.GetArguments().size() == 5U);
        CHECK(program_bindings.GetFrameIndex() == 2U);
        for(const auto& [program_argument, argument_binding_values] : compute_resource_views)
        {
            const Rhi::ResourceViews& program_resource_views = program_bindings.Get(program_argument).GetResourceViews();
            REQUIRE(program_resource_views.size() > 0);
            CHECK(program_resource_views.at(0) == std::get<Rhi::ResourceView>(argument_binding_values));
        }
    }

    SECTION("Destroy Program Bindings after Release")
    {
        Rhi::ProgramBindings program_bindings;
        REQUIRE_NOTHROW(program_bindings = compute_program.CreateBindings(compute_resource_views, 2U));
        REQUIRE(program_bindings.IsInitialized());

        WeakPtr<Rhi::IProgramBindings> program_bindings_wptr = program_bindings.GetInterfacePtr();
        CHECK(program_bindings_wptr.use_count() == 1);
        REQUIRE_NOTHROW(program_bindings = {});
        CHECK(program_bindings_wptr.expired());
    }

    SECTION("Can not create Compute Program Bindings with Unbound Resources")
    {
        Rhi::ProgramBindings program_bindings;
        REQUIRE_THROWS_AS(program_bindings = compute_program.CreateBindings({
                { { Rhi::ShaderType::Compute, "InTexture" }, texture1.GetResourceView() },
                { { Rhi::ShaderType::Compute, "OutBuffer" }, buffer1.GetResourceView() }
            }),
            Rhi::ProgramBindingsUnboundArgumentsException);
    }

    SECTION("Create Multiple Compute Program Bindings")
    {
        std::vector<Rhi::ProgramBindings> program_bindings;
        for(size_t i = 0; i < 10; ++i)
        {
            REQUIRE_NOTHROW(program_bindings.push_back(compute_program.CreateBindings(compute_resource_views)));
            REQUIRE(program_bindings.back().IsInitialized());
            CHECK(program_bindings.back().GetArguments().size() == 5U);
            CHECK(program_bindings.back().GetBindingsIndex() == i);
        }
        CHECK(compute_program.GetBindingsCount() == 10);
        program_bindings.clear();
        CHECK(compute_program.GetBindingsCount() == 0);
    }

    SECTION("Create A Copy of Program Bindings with Replacements")
    {
        Rhi::ProgramBindings orig_program_bindings = compute_program.CreateBindings(compute_resource_views, 2U);
        Rhi::ProgramBindings copy_program_bindings;
        REQUIRE_NOTHROW(copy_program_bindings = Rhi::ProgramBindings(orig_program_bindings, {
            { { Rhi::ShaderType::Compute, "OutBuffer" }, buffer2.GetResourceView() },
        }, 3U));
        REQUIRE(copy_program_bindings.IsInitialized());
        CHECK(copy_program_bindings.GetInterfacePtr());
        CHECK(copy_program_bindings.GetArguments().size() == 5U);
        CHECK(copy_program_bindings.GetFrameIndex() == 3U);
        CHECK(copy_program_bindings.Get({ Rhi::ShaderType::Compute, "InTexture" }).GetResourceViews().at(0).GetResourcePtr().get() == texture1.GetInterfacePtr().get());
        CHECK(copy_program_bindings.Get({ Rhi::ShaderType::Compute, "InSampler" }).GetResourceViews().at(0).GetResourcePtr().get() == sampler.GetInterfacePtr().get());
        CHECK(copy_program_bindings.Get({ Rhi::ShaderType::Compute, "OutBuffer" }).GetResourceViews().at(0).GetResourcePtr().get() == buffer2.GetInterfacePtr().get());
    }

    SECTION("Object Destroyed Callback")
    {
        Rhi::ProgramBindings program_bindings = compute_program.CreateBindings(compute_resource_views);
        ObjectCallbackTester object_callback_tester(program_bindings);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        program_bindings = {};
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    const Rhi::ProgramBindings program_bindings = compute_program.CreateBindings(compute_resource_views);

    SECTION("Object Name Setup")
    {
        CHECK(program_bindings.SetName("My Program Bindings"));
        CHECK(program_bindings.GetName() == "My Program Bindings");
    }

    SECTION("Object Name Change Callback")
    {
        CHECK(program_bindings.SetName("My Program Bindings"));
        ObjectCallbackTester object_callback_tester(program_bindings);
        CHECK(program_bindings.SetName("Our Program Bindings"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Program Bindings");
        CHECK(object_callback_tester.GetOldObjectName() == "My Program Bindings");
    }

    SECTION("Object Name Set Unchanged")
    {
        const Rhi::ProgramBindings program_bindings = compute_program.CreateBindings(compute_resource_views);
        CHECK(program_bindings.SetName("My Program Bindings"));
        ObjectCallbackTester object_callback_tester(program_bindings);
        CHECK_FALSE(program_bindings.SetName("My Program Bindings"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Can Get Program Binding Arguments")
    {
        const Rhi::ProgramBindings program_bindings = compute_program.CreateBindings(compute_resource_views);
        REQUIRE(program_bindings.GetArguments().size() == 5U);
        Rhi::ProgramArguments program_arguments;
        REQUIRE_NOTHROW(program_arguments = program_bindings.GetArguments());
        CHECK(program_arguments.count({ Rhi::ShaderType::Compute, "InBuffer" }) == 1);
        CHECK(program_arguments.count({ Rhi::ShaderType::Compute, "InValue" }) == 1);
        CHECK(program_arguments.count({ Rhi::ShaderType::Compute, "InTexture" }) == 1);
        CHECK(program_arguments.count({ Rhi::ShaderType::Compute, "InSampler" }) == 1);
        CHECK(program_arguments.count({ Rhi::ShaderType::Compute, "OutBuffer" }) == 1);
    }

    SECTION("Can Get Texture Argument Binding")
    {
        const Rhi::ProgramBindings program_bindings = compute_program.CreateBindings(compute_resource_views);
        Rhi::IProgramArgumentBinding* texture_binding_ptr = nullptr;
        REQUIRE_NOTHROW(texture_binding_ptr = &program_bindings.Get({ Rhi::ShaderType::Compute, "InTexture" }));
        REQUIRE(texture_binding_ptr);
        CHECK(texture_binding_ptr->GetSettings().argument.GetName() == "InTexture");
        CHECK(texture_binding_ptr->GetSettings().resource_count == 1U);
        CHECK(texture_binding_ptr->GetSettings().resource_type == Rhi::ResourceType::Texture);
        CHECK(texture_binding_ptr->GetResourceViews().size() == 1U);
        CHECK(texture_binding_ptr->GetResourceViews().at(0).GetResourcePtr().get() == texture1.GetInterfacePtr().get());
    }

    SECTION("Can Get Sampler Argument Binding")
    {
        const Rhi::ProgramBindings program_bindings = compute_program.CreateBindings(compute_resource_views);
        Rhi::IProgramArgumentBinding* sampler_binding_ptr = nullptr;
        REQUIRE_NOTHROW(sampler_binding_ptr = &program_bindings.Get({ Rhi::ShaderType::Compute, "InSampler" }));
        REQUIRE(sampler_binding_ptr);
        CHECK(sampler_binding_ptr->GetSettings().argument.GetName() == "InSampler");
        CHECK(sampler_binding_ptr->GetSettings().resource_count == 1U);
        CHECK(sampler_binding_ptr->GetSettings().resource_type == Rhi::ResourceType::Sampler);
        CHECK(sampler_binding_ptr->GetResourceViews().size() == 1U);
        CHECK(sampler_binding_ptr->GetResourceViews().at(0).GetResourcePtr().get() == sampler.GetInterfacePtr().get());
    }

    SECTION("Can Get Buffer Argument Binding")
    {

        Rhi::IProgramArgumentBinding* buffer_binding_ptr = nullptr;
        REQUIRE_NOTHROW(buffer_binding_ptr = &program_bindings.Get({ Rhi::ShaderType::Compute, "OutBuffer" }));
        REQUIRE(buffer_binding_ptr);
        CHECK(buffer_binding_ptr->GetSettings().argument.GetName() == "OutBuffer");
        CHECK(buffer_binding_ptr->GetSettings().resource_count == 1U);
        CHECK(buffer_binding_ptr->GetSettings().resource_type == Rhi::ResourceType::Buffer);
        CHECK(buffer_binding_ptr->GetResourceViews().size() == 1U);
        CHECK(buffer_binding_ptr->GetResourceViews().at(0).GetResourcePtr().get() == buffer1.GetInterfacePtr().get());
    }

    SECTION("Can not Get Non-Existing Argument Binding")
    {
        CHECK_THROWS_AS(program_bindings.Get({ Rhi::ShaderType::Compute, "NonExisting" }), Rhi::ProgramArgumentNotFoundException);
        CHECK_THROWS_AS(program_bindings.Get({ Rhi::ShaderType::All, "OutBuffer" }), Rhi::ProgramArgumentNotFoundException);
        CHECK_THROWS_AS(program_bindings.Get({ Rhi::ShaderType::Pixel, "InSampler" }), Rhi::ProgramArgumentNotFoundException);
    }

    SECTION("Can Change Buffer Argument Binding")
    {
        Rhi::IProgramArgumentBinding& buffer_binding = program_bindings.Get({ Rhi::ShaderType::Compute, "OutBuffer" });
        const Rhi::ResourceView buffer2_view(buffer2.GetInterface(), 0U, 128U);
        REQUIRE_NOTHROW(buffer_binding.SetResourceView(buffer2_view));
        CHECK(buffer_binding.GetResourceViews().at(0) == buffer2_view);
    }

    SECTION("Can Change Texture Argument Binding")
    {
        Rhi::IProgramArgumentBinding& texture_binding = program_bindings.Get({ Rhi::ShaderType::Compute, "InTexture" });
        REQUIRE_NOTHROW(texture_binding.SetResourceView(texture2.GetResourceView()));
        CHECK(texture_binding.GetResourceViews().at(0) == texture2.GetResourceView());
    }

    SECTION("Can Set Root-Constant Buffer")
    {
        Rhi::IProgramArgumentBinding& root_constant_binding = program_bindings.Get({ Rhi::ShaderType::Compute, "InBuffer" });
        const Rhi::RootConstant test_root_constant(42U);
        REQUIRE_NOTHROW(root_constant_binding.SetRootConstant(test_root_constant));
        CHECK(root_constant_binding.GetResourceViews().size() == 1U);
        CHECK(root_constant_binding.GetRootConstant() == test_root_constant);
        CHECK(root_constant_binding.GetRootConstant().GetValue<uint32_t>() == 42U);
    }

    SECTION("Can Set Root-Constant Value")
    {
        Rhi::IProgramArgumentBinding& root_constant_binding = program_bindings.Get({ Rhi::ShaderType::Compute, "InValue" });
        const Rhi::RootConstant test_root_constant(36U);
        REQUIRE_NOTHROW(root_constant_binding.SetRootConstant(test_root_constant));
        CHECK(root_constant_binding.GetResourceViews().empty());
        CHECK(root_constant_binding.GetRootConstant() == test_root_constant);
        CHECK(root_constant_binding.GetRootConstant().GetValue<uint32_t>() == 36U);
    }

    SECTION("Convert to String")
    {
        program_bindings.Get({ Rhi::ShaderType::Compute, "InBuffer" }).SetRootConstant(Rhi::RootConstant(42U));
        CHECK(static_cast<std::string>(program_bindings) ==
              "  - Compute shaders argument 'InBuffer' (Constant, RootConstantBuffer) is bound to Buffer 'Program Root Constant Buffer' subresources from index(d:0, a:0, m:0) for count(d:1, a:1, m:1) with offset 0;\n" \
              "  - Compute shaders argument 'InSampler' (Constant, ResourceView) is bound to Sampler 'S' subresources from index(d:0, a:0, m:0) for count(d:0, a:0, m:0) with offset 0;\n" \
              "  - Compute shaders argument 'InTexture' (Mutable, ResourceView) is bound to Texture 'T1' subresources from index(d:0, a:0, m:0) for count(d:1, a:1, m:1) with offset 0;\n" \
              "  - Compute shaders argument 'InValue' (Mutable, RootConstantValue) is bound to value of 4 bytes;\n" \
              "  - Compute shaders argument 'OutBuffer' (Mutable, ResourceView) is bound to Buffer 'B1' subresources from index(d:0, a:0, m:0) for count(d:1, a:1, m:1) with offset 0.");
    }
}