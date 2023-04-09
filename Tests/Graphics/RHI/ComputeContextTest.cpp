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

#include "Methane/Graphics/RHI/ICommandList.h"
#include "Methane/Graphics/RHI/IContext.h"
#include "RhiTestHelpers.hpp"

#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/CommandKit.h>
#include <Methane/Graphics/RHI/TransferCommandList.h>
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
    static const Rhi::Devices& devices = Rhi::System::Get().UpdateGpuDevices();
    if (devices.empty())
        throw std::logic_error("No RHI devices available");
    return devices[0];
}

TEST_CASE("RHI Compute Context", "[rhi][compute][context]")
{
    const Rhi::ComputeContextSettings compute_context_settings{
        Rhi::ContextOptionMask{ Rhi::ContextOption::TransferWithD3D12DirectQueue }
    };

    SECTION("Context Construction")
    {
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
        CHECK(compute_context.IsInitialized());
        CHECK(compute_context.GetInterfacePtr());
        CHECK(compute_context.GetInterface().GetSettings() == compute_context_settings);
        CHECK(compute_context.GetOptions() == compute_context_settings.options);
        CHECK(compute_context.GetName() == "");
        CHECK(compute_context.GetDevice() == GetTestDevice());
        CHECK(std::addressof(compute_context.GetParallelExecutor()) == std::addressof(g_parallel_executor));
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
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
        CHECK(compute_context.SetName("My Compute Context"));
        CHECK(compute_context.GetName() == "My Compute Context");
    }

    SECTION("Object Name Change Callback")
    {
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
        CHECK_NOTHROW(compute_context.SetName("My Compute Context"));
        ObjectCallbackTester object_callback_tester(compute_context.GetInterface());
        CHECK(compute_context.SetName("Our Compute Context"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Compute Context");
        CHECK(object_callback_tester.GetOldObjectName() == "My Compute Context");
    }

    SECTION("Object Name Set Unchanged")
    {
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
        CHECK_NOTHROW(compute_context.SetName("My Compute Context"));
        ObjectCallbackTester object_callback_tester(compute_context.GetInterface());
        CHECK_FALSE(compute_context.SetName("My Compute Context"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Context Reset")
    {
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
        ContextCallbackTester context_callback_tester(compute_context.GetInterface());
        CHECK_NOTHROW(compute_context.Reset());
        CHECK(context_callback_tester.IsContextReleased());
        CHECK_FALSE(context_callback_tester.IsContextCompletingInitialization());
        CHECK(context_callback_tester.IsContextInitialized());
    }

    SECTION("Context Reset with Device")
    {
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
        ContextCallbackTester context_callback_tester(compute_context.GetInterface());
        const Rhi::Device new_device = Rhi::System::Get().UpdateGpuDevices().at(0);
        CHECK_NOTHROW(compute_context.Reset(new_device));
        CHECK(context_callback_tester.IsContextReleased());
        CHECK_FALSE(context_callback_tester.IsContextCompletingInitialization());
        CHECK(context_callback_tester.IsContextInitialized());
        CHECK(compute_context.GetDevice() == new_device);
    }

    SECTION("Context Upload Command Kit")
    {
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
        Rhi::CommandKit upload_cmd_kit;
        CHECK_NOTHROW(upload_cmd_kit = compute_context.GetUploadCommandKit());
        CHECK(upload_cmd_kit.IsInitialized());
        CHECK(upload_cmd_kit.GetListType() == Rhi::CommandListType::Transfer);
    }

    SECTION("Context Compute Command Kit")
    {
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
        Rhi::CommandKit compute_cmd_kit;
        CHECK_NOTHROW(compute_cmd_kit = compute_context.GetComputeCommandKit());
        CHECK(compute_cmd_kit.IsInitialized());
        CHECK(compute_cmd_kit.GetListType() == Rhi::CommandListType::Compute);
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
        ContextCallbackTester context_callback_tester(compute_context.GetInterface());
        CHECK_NOTHROW(compute_context.CompleteInitialization());
        CHECK(context_callback_tester.IsContextCompletingInitialization());
    }

    SECTION("Context Complete Initialization Deferred")
    {
        const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, compute_context_settings);
        ContextCallbackTester context_callback_tester(compute_context.GetInterface());
        Rhi::TransferCommandList transfer_cmd_list = compute_context.GetUploadCommandKit().GetTransferListForEncoding();
        CHECK(transfer_cmd_list.GetState() == Rhi::CommandListState::Encoding);
        CHECK_NOTHROW(compute_context.RequestDeferredAction(Rhi::ContextDeferredAction::CompleteInitialization));
        CHECK_NOTHROW(compute_context.WaitForGpu(Rhi::ContextWaitFor::ComputeComplete));
        //FIXME: CHECK(transfer_cmd_list.GetState() == Rhi::CommandListState::Executing);
    }
}
