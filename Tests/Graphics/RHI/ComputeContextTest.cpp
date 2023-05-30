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

FILE: Tests/Graphics/RHI/ComputeContextTest.cpp
Unit-tests of the RHI ComputeContext

******************************************************************************/

#include "RhiTestHelpers.hpp"

#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/CommandKit.h>
#include <Methane/Graphics/RHI/TransferCommandList.h>
#include <Methane/Graphics/RHI/System.h>
#include <Methane/Graphics/RHI/Device.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/ComputeState.h>
#include <Methane/Graphics/RHI/Program.h>
#include <Methane/Graphics/RHI/Shader.h>
#include <Methane/Graphics/RHI/Buffer.h>
#include <Methane/Graphics/RHI/Texture.h>
#include <Methane/Graphics/RHI/Sampler.h>

#include <taskflow/taskflow.hpp>
#include <magic_enum.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

TEST_CASE("RHI Compute Context Functions", "[rhi][compute][context]")
{
    const Rhi::ComputeContextSettings compute_context_settings{
        Rhi::ContextOptionMask{ Rhi::ContextOption::TransferWithD3D12DirectQueue }
    };

    SECTION("Context Construction")
    {
        Rhi::ComputeContext compute_context;
        REQUIRE_NOTHROW(compute_context = Rhi::ComputeContext(GetTestDevice(), g_parallel_executor, compute_context_settings));
        REQUIRE(compute_context.IsInitialized());
        CHECK(compute_context.GetInterfacePtr());
        CHECK(compute_context.GetSettings() == compute_context_settings);
        CHECK(compute_context.GetOptions() == compute_context_settings.options);
        CHECK(compute_context.GetName() == "");
        CHECK(compute_context.GetDevice() == GetTestDevice());
        CHECK(std::addressof(compute_context.GetParallelExecutor()) == std::addressof(g_parallel_executor));
    }

    SECTION("Object Destroyed Callback")
    {
        auto compute_context_ptr = std::make_unique<Rhi::ComputeContext>(GetTestDevice(), g_parallel_executor, compute_context_settings);
        ObjectCallbackTester object_callback_tester(*compute_context_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        compute_context_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);

    SECTION("Object Name Setup")
    {
        CHECK(compute_context.SetName("My Compute Context"));
        CHECK(compute_context.GetName() == "My Compute Context");
    }

    SECTION("Object Name Change Callback")
    {
        CHECK(compute_context.SetName("My Compute Context"));
        ObjectCallbackTester object_callback_tester(compute_context);
        CHECK(compute_context.SetName("Our Compute Context"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Compute Context");
        CHECK(object_callback_tester.GetOldObjectName() == "My Compute Context");
    }

    SECTION("Object Name Set Unchanged")
    {
        CHECK(compute_context.SetName("My Compute Context"));
        ObjectCallbackTester object_callback_tester(compute_context);
        CHECK_FALSE(compute_context.SetName("My Compute Context"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Context Reset")
    {
        ContextCallbackTester context_callback_tester(compute_context);
        REQUIRE_NOTHROW(compute_context.Reset());
        CHECK(context_callback_tester.IsContextReleased());
        CHECK_FALSE(context_callback_tester.IsContextCompletingInitialization());
        CHECK(context_callback_tester.IsContextInitialized());
    }

    SECTION("Context Reset with Device")
    {
        ContextCallbackTester context_callback_tester(compute_context);
        const Rhi::Device new_device = Rhi::System::Get().UpdateGpuDevices().at(0);
        REQUIRE_NOTHROW(compute_context.Reset(new_device));
        CHECK(context_callback_tester.IsContextReleased());
        CHECK_FALSE(context_callback_tester.IsContextCompletingInitialization());
        CHECK(context_callback_tester.IsContextInitialized());
        CHECK(compute_context.GetDevice() == new_device);
    }

    SECTION("Context Upload Command Kit")
    {
        Rhi::CommandKit upload_cmd_kit;
        REQUIRE_NOTHROW(upload_cmd_kit = compute_context.GetUploadCommandKit());
        REQUIRE(upload_cmd_kit.IsInitialized());
        CHECK(upload_cmd_kit.GetListType() == Rhi::CommandListType::Transfer);
    }

    SECTION("Context Compute Command Kit")
    {
        Rhi::CommandKit compute_cmd_kit;
        REQUIRE_NOTHROW(compute_cmd_kit = compute_context.GetComputeCommandKit());
        REQUIRE(compute_cmd_kit.IsInitialized());
        CHECK(compute_cmd_kit.GetListType() == Rhi::CommandListType::Compute);
    }

    SECTION("Context Compute Command Kit")
    {
        Rhi::CommandKit compute_cmd_kit;
        REQUIRE_NOTHROW(compute_cmd_kit = compute_context.GetComputeCommandKit());
        REQUIRE(compute_cmd_kit.IsInitialized());
        CHECK(compute_cmd_kit.GetListType() == Rhi::CommandListType::Compute);
    }

    SECTION("Context Default Command Kits")
    {
        const std::array<Rhi::CommandListType, 2> cmd_list_types { Rhi::CommandListType::Compute, Rhi::CommandListType::Transfer };
        for(Rhi::CommandListType cmd_list_type : cmd_list_types)
        {
            Rhi::CommandKit default_cmd_kit = compute_context.GetDefaultCommandKit(cmd_list_type);
            CHECK(default_cmd_kit.IsInitialized());
            CHECK(default_cmd_kit.GetListType() == cmd_list_type);
        }
    }

    SECTION("Context Upload Resources")
    {
        Rhi::TransferCommandList transfer_cmd_list = compute_context.GetUploadCommandKit().GetTransferListForEncoding();
        CHECK(transfer_cmd_list.GetState() == Rhi::CommandListState::Encoding);
        CHECK_NOTHROW(compute_context.UploadResources());
        CHECK(transfer_cmd_list.GetState() == Rhi::CommandListState::Executing);
    }

    SECTION("Context Upload Resources Deferred")
    {
        Rhi::TransferCommandList transfer_cmd_list = compute_context.GetUploadCommandKit().GetTransferListForEncoding();
        CHECK(transfer_cmd_list.GetState() == Rhi::CommandListState::Encoding);
        CHECK_NOTHROW(compute_context.RequestDeferredAction(Rhi::ContextDeferredAction::UploadResources));
        CHECK_NOTHROW(compute_context.WaitForGpu(Rhi::ContextWaitFor::ComputeComplete));
        //FIXME: CHECK(transfer_cmd_list.GetState() == Rhi::CommandListState::Executing);
    }

    SECTION("Context Complete Initialization")
    {
        ContextCallbackTester context_callback_tester(compute_context);
        REQUIRE_NOTHROW(compute_context.CompleteInitialization());
        CHECK(context_callback_tester.IsContextCompletingInitialization());
        CHECK_FALSE(compute_context.IsCompletingInitialization());
    }

    SECTION("Context Complete Initialization Deferred")
    {
        ContextCallbackTester context_callback_tester(compute_context);
        Rhi::TransferCommandList transfer_cmd_list = compute_context.GetUploadCommandKit().GetTransferListForEncoding();
        CHECK(transfer_cmd_list.GetState() == Rhi::CommandListState::Encoding);
        CHECK_NOTHROW(compute_context.RequestDeferredAction(Rhi::ContextDeferredAction::CompleteInitialization));
        CHECK_NOTHROW(compute_context.WaitForGpu(Rhi::ContextWaitFor::ComputeComplete));
        //FIXME: CHECK(transfer_cmd_list.GetState() == Rhi::CommandListState::Executing);
    }
}

TEST_CASE("RHI Compute Context Factory", "[rhi][compute][context][factory]")
{
    const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, {});

    SECTION("Can Create Compute Command Queue")
    {
        Rhi::CommandQueue command_queue;
        REQUIRE_NOTHROW(command_queue = compute_context.CreateCommandQueue(Rhi::CommandListType::Compute));
        REQUIRE(command_queue.IsInitialized());
        CHECK(command_queue.GetCommandListType() == Rhi::CommandListType::Compute);
    }

    SECTION("Can Create Transfer Command Queue")
    {
        Rhi::CommandQueue command_queue;
        REQUIRE_NOTHROW(command_queue = compute_context.CreateCommandQueue(Rhi::CommandListType::Transfer));
        REQUIRE(command_queue.IsInitialized());
        CHECK(command_queue.GetCommandListType() == Rhi::CommandListType::Transfer);
    }

    SECTION("Can not Create Render Command Queue")
    {
        Rhi::CommandQueue command_queue;
        REQUIRE_THROWS(command_queue = compute_context.CreateCommandQueue(Rhi::CommandListType::Render));
    }

    SECTION("Can Create Compute Command Kit")
    {
        Rhi::CommandKit command_kit;
        REQUIRE_NOTHROW(command_kit = compute_context.CreateCommandKit(Rhi::CommandListType::Compute));
        REQUIRE(command_kit.IsInitialized());
        CHECK(command_kit.GetListType() == Rhi::CommandListType::Compute);
    }

    SECTION("Can Create Transfer Command Kit")
    {
        Rhi::CommandKit command_kit;
        REQUIRE_NOTHROW(command_kit = compute_context.CreateCommandKit(Rhi::CommandListType::Transfer));
        REQUIRE(command_kit.IsInitialized());
        CHECK(command_kit.GetListType() == Rhi::CommandListType::Transfer);
    }

    SECTION("Can not Create Render Command Kit")
    {
        Rhi::CommandKit command_kit;
        REQUIRE_THROWS(command_kit = compute_context.CreateCommandKit(Rhi::CommandListType::Render));
    }

    SECTION("Can Create Shader")
    {
        const Rhi::ShaderSettings shader_settings{ Data::ShaderProvider::Get(), { "Shader", "Main" } };
        Rhi::Shader shader;
        REQUIRE_NOTHROW(shader = compute_context.CreateShader(Rhi::ShaderType::Compute, shader_settings));
        REQUIRE(shader.IsInitialized());
        CHECK(shader.GetType() == Rhi::ShaderType::Compute);
        CHECK(shader.GetSettings().entry_function == shader_settings.entry_function);
    }

    SECTION("Can Create Program")
    {
        Rhi::Program program;
        REQUIRE_NOTHROW(program = compute_context.CreateProgram({
            { { Rhi::ShaderType::Compute, { Data::ShaderProvider::Get(), { "Shader", "Main" } } } },
        }));
        REQUIRE(program.IsInitialized());
        CHECK(program.GetSettings().shaders.size() == 1);
        CHECK(program.GetShader(Rhi::ShaderType::Compute).GetSettings().entry_function == Rhi::ShaderEntryFunction{ "Shader", "Main" });
    }

    SECTION("Can Create Compute State")
    {
        const Rhi::ComputeStateSettingsImpl& compute_state_settings{
            compute_context.CreateProgram({
                { { Rhi::ShaderType::Compute, { Data::ShaderProvider::Get(), { "Shader", "Main" } } } },
            }),
            Rhi::ThreadGroupSize(16, 16, 1)
        };
        Rhi::ComputeState compute_state;
        REQUIRE_NOTHROW(compute_state = compute_context.CreateComputeState(compute_state_settings));
        REQUIRE(compute_state.IsInitialized());
        CHECK(compute_state.GetSettings().program_ptr == compute_state_settings.program.GetInterfacePtr());
        CHECK(compute_state.GetSettings().thread_group_size == compute_state_settings.thread_group_size);
    }

    SECTION("Can Create Buffer")
    {
        const Rhi::BufferSettings buffer_settings = Rhi::BufferSettings::ForConstantBuffer(42);
        Rhi::Buffer buffer;
        REQUIRE_NOTHROW(buffer = compute_context.CreateBuffer(buffer_settings));
        REQUIRE(buffer.IsInitialized());
        CHECK(buffer.GetSettings().type == Rhi::BufferType::Constant);
        CHECK(buffer.GetSettings().size == buffer_settings.size);
    }

    SECTION("Can Create Texture")
    {
        const Rhi::TextureSettings buffer_settings = Rhi::TextureSettings::ForImage(Dimensions(640, 480), 2, PixelFormat::RGBA8Unorm_sRGB, true);
        Rhi::Texture texture;
        REQUIRE_NOTHROW(texture = compute_context.CreateTexture(buffer_settings));
        REQUIRE(texture.IsInitialized());
        CHECK(texture.GetSettings().type == Rhi::TextureType::Image);
        CHECK(texture.GetSettings().dimension_type == Rhi::TextureDimensionType::Tex2DArray);
        CHECK(texture.GetSettings().array_length == 2);
        CHECK(texture.GetSettings().dimensions == Dimensions(640, 480));
        CHECK(texture.GetSettings().pixel_format == PixelFormat::RGBA8Unorm_sRGB);
        CHECK(texture.GetSettings().mipmapped);
    }

    SECTION("Can Create Sampler")
    {
        const Rhi::SamplerSettings sampler_settings{
            rhi::SamplerFilter  { rhi::SamplerFilter::MinMag::Linear },
            rhi::SamplerAddress { rhi::SamplerAddress::Mode::ClampToEdge }
        };
        Rhi::Sampler sampler;
        REQUIRE_NOTHROW(sampler = compute_context.CreateSampler(sampler_settings));
        REQUIRE(sampler.IsInitialized());
        CHECK(sampler.GetSettings().filter.min == Rhi::SamplerFilter::MinMag::Linear);
        CHECK(sampler.GetSettings().filter.mag == Rhi::SamplerFilter::MinMag::Linear);
        CHECK(sampler.GetSettings().address.r == Rhi::SamplerAddress::Mode::ClampToEdge);
        CHECK(sampler.GetSettings().address.s == Rhi::SamplerAddress::Mode::ClampToEdge);
        CHECK(sampler.GetSettings().address.t == Rhi::SamplerAddress::Mode::ClampToEdge);
    }

    SECTION("Can Get Object Registry")
    {
        Rhi::IObjectRegistry* registry_ptr = nullptr;
        REQUIRE_NOTHROW(registry_ptr = &compute_context.GetObjectRegistry());
        REQUIRE(registry_ptr);
        CHECK_FALSE(registry_ptr->HasGraphicsObject("Something"));
    }

    SECTION("Can Get Parallel Executor")
    {
        tf::Executor* executor_ptr = nullptr;
        REQUIRE_NOTHROW(executor_ptr = &compute_context.GetParallelExecutor());
        REQUIRE(executor_ptr);
        CHECK(executor_ptr->num_workers() > 0);
    }
}
