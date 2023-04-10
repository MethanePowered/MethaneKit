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

#include "Methane/Graphics/RHI/IShader.h"
#include "Methane/Graphics/RHI/Shader.h"
#include "RhiTestHelpers.hpp"

#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/CommandKit.h>
#include <Methane/Graphics/RHI/TransferCommandList.h>
#include <Methane/Graphics/RHI/System.h>
#include <Methane/Graphics/RHI/Device.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/CommandKit.h>
#include <Methane/Graphics/RHI/ComputeState.h>

#include <memory>
#include <stdexcept>
#include <taskflow/taskflow.hpp>
#include <magic_enum.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

static Rhi::Device GetTestDevice()
{
    static const Rhi::Devices& devices = Rhi::System::Get().UpdateGpuDevices();
    if (devices.empty())
        throw std::logic_error("No RHI devices available");
    return devices[0];
}

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

    SECTION("Object Name Setup")
    {
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
        CHECK(compute_context.SetName("My Compute Context"));
        CHECK(compute_context.GetName() == "My Compute Context");
    }

    SECTION("Object Name Change Callback")
    {
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
        CHECK(compute_context.SetName("My Compute Context"));
        ObjectCallbackTester object_callback_tester(compute_context);
        CHECK(compute_context.SetName("Our Compute Context"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Compute Context");
        CHECK(object_callback_tester.GetOldObjectName() == "My Compute Context");
    }

    SECTION("Object Name Set Unchanged")
    {
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
        CHECK(compute_context.SetName("My Compute Context"));
        ObjectCallbackTester object_callback_tester(compute_context);
        CHECK_FALSE(compute_context.SetName("My Compute Context"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Context Reset")
    {
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
        ContextCallbackTester context_callback_tester(compute_context);
        REQUIRE_NOTHROW(compute_context.Reset());
        CHECK(context_callback_tester.IsContextReleased());
        CHECK_FALSE(context_callback_tester.IsContextCompletingInitialization());
        CHECK(context_callback_tester.IsContextInitialized());
    }

    SECTION("Context Reset with Device")
    {
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
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
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
        Rhi::CommandKit upload_cmd_kit;
        REQUIRE_NOTHROW(upload_cmd_kit = compute_context.GetUploadCommandKit());
        REQUIRE(upload_cmd_kit.IsInitialized());
        CHECK(upload_cmd_kit.GetListType() == Rhi::CommandListType::Transfer);
    }

    SECTION("Context Compute Command Kit")
    {
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
        Rhi::CommandKit compute_cmd_kit;
        REQUIRE_NOTHROW(compute_cmd_kit = compute_context.GetComputeCommandKit());
        REQUIRE(compute_cmd_kit.IsInitialized());
        CHECK(compute_cmd_kit.GetListType() == Rhi::CommandListType::Compute);
    }

    SECTION("Context Compute Command Kit")
    {
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
        Rhi::CommandKit compute_cmd_kit;
        REQUIRE_NOTHROW(compute_cmd_kit = compute_context.GetComputeCommandKit());
        REQUIRE(compute_cmd_kit.IsInitialized());
        CHECK(compute_cmd_kit.GetListType() == Rhi::CommandListType::Compute);
    }

    SECTION("Context Default Command Kits")
    {
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
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
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
        Rhi::TransferCommandList transfer_cmd_list = compute_context.GetUploadCommandKit().GetTransferListForEncoding();
        CHECK(transfer_cmd_list.GetState() == Rhi::CommandListState::Encoding);
        CHECK_NOTHROW(compute_context.UploadResources());
        CHECK(transfer_cmd_list.GetState() == Rhi::CommandListState::Executing);
    }

    SECTION("Context Upload Resources Deferred")
    {
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
        Rhi::TransferCommandList transfer_cmd_list = compute_context.GetUploadCommandKit().GetTransferListForEncoding();
        CHECK(transfer_cmd_list.GetState() == Rhi::CommandListState::Encoding);
        CHECK_NOTHROW(compute_context.RequestDeferredAction(Rhi::ContextDeferredAction::UploadResources));
        CHECK_NOTHROW(compute_context.WaitForGpu(Rhi::ContextWaitFor::ComputeComplete));
        //FIXME: CHECK(transfer_cmd_list.GetState() == Rhi::CommandListState::Executing);
    }

    SECTION("Context Complete Initialization")
    {
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
        ContextCallbackTester context_callback_tester(compute_context);
        REQUIRE_NOTHROW(compute_context.CompleteInitialization());
        CHECK(context_callback_tester.IsContextCompletingInitialization());
        CHECK_FALSE(compute_context.IsCompletingInitialization());
    }

    SECTION("Context Complete Initialization Deferred")
    {
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
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
        CHECK_NOTHROW(compute_state = compute_context.CreateComputeState(compute_state_settings));
        REQUIRE(compute_state.IsInitialized());
        CHECK(compute_state.GetSettings().program_ptr == compute_state_settings.program.GetInterfacePtr());
        CHECK(compute_state.GetSettings().thread_group_size == compute_state_settings.thread_group_size);
    }
}
