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
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/CommandKit.h>
#include <Methane/Graphics/RHI/Fence.h>
#include <Methane/Graphics/RHI/TransferCommandList.h>
#include <Methane/Graphics/RHI/ComputeCommandList.h>

#include <memory>
#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

TEST_CASE("RHI Command Queue Functions", "[rhi][queue]")
{
    const Rhi::ComputeContext compute_context = Rhi::ComputeContext(GetTestDevice(), g_parallel_executor, {});

    SECTION("Command Queue Construction")
    {
        Rhi::CommandQueue cmd_queue;
        REQUIRE_NOTHROW(cmd_queue = compute_context.CreateCommandQueue(Rhi::CommandListType::Compute));
        REQUIRE(cmd_queue.IsInitialized());
        CHECK(cmd_queue.GetInterfacePtr());
        CHECK(cmd_queue.GetCommandListType() == Rhi::CommandListType::Compute);
    }

    SECTION("Object Destroyed Callback")
    {
        auto cmd_queue_ptr = std::make_unique<Rhi::CommandQueue>(compute_context, Rhi::CommandListType::Compute);
        ObjectCallbackTester object_callback_tester(*cmd_queue_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        cmd_queue_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    const Rhi::CommandQueue cmd_queue = compute_context.CreateCommandQueue(Rhi::CommandListType::Compute);

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
}

TEST_CASE("RHI Compute Command Queue Factory", "[rhi][compute][context][factory]")
{
    const Rhi::ComputeContext compute_context(GetTestDevice(), g_parallel_executor, {});
    const Rhi::CommandQueue compute_cmd_queue = compute_context.CreateCommandQueue(Rhi::CommandListType::Compute);

    SECTION("Can Create Command Kit")
    {
        Rhi::CommandKit command_kit;
        REQUIRE_NOTHROW(command_kit = compute_cmd_queue.CreateCommandKit());
        REQUIRE(command_kit.IsInitialized());
        CHECK(command_kit.GetListType() == Rhi::CommandListType::Compute);
        CHECK(command_kit.GetQueue().GetInterfacePtr().get() == compute_cmd_queue.GetInterfacePtr().get());
    }

    SECTION("Can Create Fence")
    {
        Rhi::Fence fence;
        REQUIRE_NOTHROW(fence = compute_cmd_queue.CreateFence());
        REQUIRE(fence.IsInitialized());
    }

    SECTION("Can Create Transfer Command List")
    {
        Rhi::TransferCommandList transfer_cmd_list;
        REQUIRE_NOTHROW(transfer_cmd_list = compute_cmd_queue.CreateTransferCommandList());
        REQUIRE(transfer_cmd_list.IsInitialized());
        CHECK(transfer_cmd_list.GetCommandQueue().GetInterfacePtr().get() == compute_cmd_queue.GetInterfacePtr().get());
    }

    SECTION("Can Create Compute Command List")
    {
        Rhi::ComputeCommandList compute_cmd_list;
        REQUIRE_NOTHROW(compute_cmd_list = compute_cmd_queue.CreateComputeCommandList());
        REQUIRE(compute_cmd_list.IsInitialized());
        CHECK(compute_cmd_list.GetCommandQueue().GetInterfacePtr().get() == compute_cmd_queue.GetInterfacePtr().get());
    }
}
