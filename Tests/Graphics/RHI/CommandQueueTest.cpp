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

FILE: Tests/Graphics/RHI/CommandQueueTest.cpp
Unit-tests of the RHI Command Queue

******************************************************************************/

#include "RhiTestHelpers.hpp"

#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/CommandKit.h>
#include <Methane/Graphics/RHI/Fence.h>
#include <Methane/Graphics/RHI/TransferCommandList.h>
#include <Methane/Graphics/RHI/ComputeCommandList.h>
#include <Methane/Graphics/RHI/ObjectRegistry.h>
#include <Methane/Graphics/RHI/CommandListSet.h>
#include <Methane/Graphics/Null/CommandListSet.h>

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
        CHECK(cmd_queue.SetName("My Compute Command Queue"));
        CHECK(cmd_queue.GetName() == "My Compute Command Queue");
    }

    SECTION("Object Name Change Callback")
    {
        CHECK(cmd_queue.SetName("My Compute Command Queue"));
        ObjectCallbackTester object_callback_tester(cmd_queue);
        CHECK(cmd_queue.SetName("Our Compute Command Queue"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Compute Command Queue");
        CHECK(object_callback_tester.GetOldObjectName() == "My Compute Command Queue");
    }

    SECTION("Object Name Set Unchanged")
    {
        CHECK(cmd_queue.SetName("My Compute Command Queue"));
        ObjectCallbackTester object_callback_tester(cmd_queue);
        CHECK_FALSE(cmd_queue.SetName("My Compute Command Queue"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Add to Objects Registry")
    {
        cmd_queue.SetName("Compute Command Queue");
        Rhi::ObjectRegistry registry = compute_context.GetObjectRegistry();
        registry.AddGraphicsObject(cmd_queue);
        const auto registered_cmd_queue = registry.GetGraphicsObject<Rhi::CommandQueue>("Compute Command Queue");
        REQUIRE(registered_cmd_queue.IsInitialized());
        CHECK(&registered_cmd_queue.GetInterface() == &cmd_queue.GetInterface());
    }

    SECTION("Execute Command Lists")
    {
        const Rhi::CommandQueue compute_cmd_queue = compute_context.CreateCommandQueue(Rhi::CommandListType::Compute);
        const Rhi::ComputeCommandList compute_cmd_list = compute_cmd_queue.CreateComputeCommandList();
        const Rhi::CommandListSet cmd_list_set({ compute_cmd_list.GetInterface() });

        REQUIRE_NOTHROW(compute_cmd_list.Reset());
        REQUIRE_NOTHROW(compute_cmd_list.Commit());
        CHECK(compute_cmd_list.GetState() == Rhi::CommandListState::Committed);

        Rhi::ICommandList* completed_command_list_ptr = nullptr;
        REQUIRE_NOTHROW(compute_cmd_queue.Execute(cmd_list_set,
            [&completed_command_list_ptr](Rhi::ICommandList& command_list) {
                completed_command_list_ptr = &command_list;
            }));

        CHECK(compute_cmd_list.GetState() == Rhi::CommandListState::Executing);
        CHECK_FALSE(completed_command_list_ptr);

        dynamic_cast<Null::CommandListSet&>(cmd_list_set.GetInterface()).Complete();

        CHECK(compute_cmd_list.GetState() == Rhi::CommandListState::Pending);
        CHECK(completed_command_list_ptr == compute_cmd_list.GetInterfacePtr().get());
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
