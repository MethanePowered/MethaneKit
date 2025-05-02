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

FILE: Tests/Graphics/RHI/TransferCommandListTest.cpp
Unit-tests of the RHI Transfer Command List

******************************************************************************/

#include "RhiTestHelpers.hpp"

#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/TransferCommandList.h>
#include <Methane/Graphics/RHI/CommandListDebugGroup.h>
#include <Methane/Graphics/RHI/ResourceBarriers.h>
#include <Methane/Graphics/RHI/ObjectRegistry.h>
#include <Methane/Graphics/RHI/CommandListSet.h>
#include <Methane/Graphics/Null/CommandListSet.h>
#include <Methane/Graphics/Null/CommandListDebugGroup.h>
#include <Methane/Graphics/Null/TransferCommandList.h>

#include <chrono>
#include <future>
#include <memory>
#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>
#include <thread>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

TEST_CASE("RHI Transfer Command List Functions", "[rhi][list][transfer]")
{
    const Rhi::ComputeContext compute_context = Rhi::ComputeContext(GetTestDevice(), g_parallel_executor, {});
    const Rhi::CommandQueue compute_cmd_queue = compute_context.CreateCommandQueue(Rhi::CommandListType::Compute);

    SECTION("Transfer Command List Construction")
    {
        Rhi::TransferCommandList cmd_list;
        REQUIRE_NOTHROW(cmd_list = compute_cmd_queue.CreateTransferCommandList());
        REQUIRE(cmd_list.IsInitialized());
        CHECK(cmd_list.GetInterfacePtr());
        CHECK(cmd_list.GetCommandQueue().GetInterfacePtr().get() == compute_cmd_queue.GetInterfacePtr().get());
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Pending);
    }

    SECTION("Object Destroyed Callback")
    {
        auto cmd_list_ptr = std::make_unique<Rhi::TransferCommandList>(compute_cmd_queue);
        ObjectCallbackTester object_callback_tester(*cmd_list_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        cmd_list_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    const Rhi::TransferCommandList cmd_list = compute_cmd_queue.CreateTransferCommandList();
    auto& null_cmd_list = dynamic_cast<Null::TransferCommandList&>(cmd_list.GetInterface());

    SECTION("Object Name Setup")
    {
        CHECK(cmd_list.SetName("My Command List"));
        CHECK(cmd_list.GetName() == "My Command List");
    }

    SECTION("Object Name Change Callback")
    {
        CHECK(cmd_list.SetName("My Command List"));
        ObjectCallbackTester object_callback_tester(cmd_list);
        CHECK(cmd_list.SetName("Our Command List"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Command List");
        CHECK(object_callback_tester.GetOldObjectName() == "My Command List");
    }

    SECTION("Object Name Set Unchanged")
    {
        CHECK(cmd_list.SetName("My Fence"));
        ObjectCallbackTester object_callback_tester(cmd_list);
        CHECK_FALSE(cmd_list.SetName("My Fence"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Add to Objects Registry")
    {
        cmd_list.SetName("Transfer Command List");
        Rhi::ObjectRegistry registry = compute_context.GetObjectRegistry();
        registry.AddGraphicsObject(cmd_list);
        const auto registered_cmd_list = registry.GetGraphicsObject<Rhi::TransferCommandList>("Transfer Command List");
        REQUIRE(registered_cmd_list.IsInitialized());
        CHECK(&registered_cmd_list.GetInterface() == &cmd_list.GetInterface());
    }

    SECTION("Reset Command List")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);
    }

    SECTION("Reset Command List Once")
    {
        REQUIRE_NOTHROW(cmd_list.ResetOnce());
        REQUIRE_NOTHROW(cmd_list.ResetOnce());
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);
    }

    SECTION("Reset Command List with Debug Group")
    {
        const Rhi::CommandListDebugGroup debug_group("Test");
        REQUIRE_NOTHROW(cmd_list.Reset(&debug_group));
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);
        REQUIRE(null_cmd_list.GetTopOpenDebugGroup() != nullptr);
        CHECK(null_cmd_list.GetTopOpenDebugGroup()->GetName() == "Test");
    }

    SECTION("Reset Command List Once with Debug Group")
    {
        const Rhi::CommandListDebugGroup debug_group("Test");
        REQUIRE_NOTHROW(cmd_list.ResetOnce(&debug_group));
        REQUIRE_NOTHROW(cmd_list.ResetOnce(&debug_group));
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);
        REQUIRE(null_cmd_list.GetTopOpenDebugGroup() != nullptr);
        CHECK(null_cmd_list.GetTopOpenDebugGroup()->GetName() == "Test");
    }

    SECTION("Push and Pop Debug Group")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK_NOTHROW(cmd_list.PushDebugGroup(Rhi::CommandListDebugGroup("Test")));
        CHECK_NOTHROW(cmd_list.PopDebugGroup());
        REQUIRE(null_cmd_list.GetTopOpenDebugGroup() == nullptr);
    }

    SECTION("Can not Pop Missing Debug Group")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK_THROWS(cmd_list.PopDebugGroup());
    }

    SECTION("Set Resource Barriers")
    {
        const Rhi::ResourceBarriers barriers(Rhi::IResourceBarriers::Set{});
        REQUIRE_NOTHROW(cmd_list.Reset());
        REQUIRE_NOTHROW(cmd_list.SetResourceBarriers(barriers.GetInterface()));
    }

    SECTION("Commit Command List")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK_NOTHROW(cmd_list.Commit());
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Committed);
    }

    SECTION("Execute Command List with Callback Tracker")
    {
        CommandListCallbackTester cmd_list_callback_tester(cmd_list);
        const Rhi::CommandListSet cmd_list_set({ cmd_list.GetInterface() });

        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);
        CHECK(cmd_list_callback_tester.GetTrackingState() == Rhi::CommandListState::Encoding);
        CHECK(cmd_list_callback_tester.IsStateChanged());
        CHECK_FALSE(cmd_list_callback_tester.IsExecutionCompleted());

        cmd_list_callback_tester.Reset();

        REQUIRE_NOTHROW(cmd_list.Commit());
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Committed);
        CHECK(cmd_list_callback_tester.GetTrackingState() == Rhi::CommandListState::Committed);
        CHECK(cmd_list_callback_tester.IsStateChanged());
        CHECK_FALSE(cmd_list_callback_tester.IsExecutionCompleted());

        cmd_list_callback_tester.Reset();

        Rhi::ICommandList* completed_command_list_ptr = nullptr;
        REQUIRE_NOTHROW(compute_cmd_queue.Execute(cmd_list_set,
            [&completed_command_list_ptr](Rhi::ICommandList& command_list) {
                completed_command_list_ptr = &command_list;
            }));

        CHECK(cmd_list.GetState() == Rhi::CommandListState::Executing);
        CHECK(cmd_list_callback_tester.GetTrackingState() == Rhi::CommandListState::Executing);
        CHECK(cmd_list_callback_tester.IsStateChanged());
        CHECK_FALSE(cmd_list_callback_tester.IsExecutionCompleted());
        CHECK_FALSE(completed_command_list_ptr);

        cmd_list_callback_tester.Reset();
        dynamic_cast<Null::CommandListSet&>(cmd_list_set.GetInterface()).Complete();

        CHECK(cmd_list.GetState() == Rhi::CommandListState::Pending);
        CHECK(cmd_list_callback_tester.GetTrackingState() == Rhi::CommandListState::Pending);
        CHECK(cmd_list_callback_tester.IsExecutionCompleted());
        CHECK(completed_command_list_ptr == cmd_list.GetInterfacePtr().get());
    }

    SECTION("Wait Until Command List Completed")
    {
        const Rhi::CommandListSet cmd_list_set({ cmd_list.GetInterface() });
        REQUIRE_NOTHROW(cmd_list.Reset());
        REQUIRE_NOTHROW(cmd_list.Commit());
        REQUIRE_NOTHROW(compute_cmd_queue.Execute(cmd_list_set));

        auto async_complete = std::async(std::launch::async, [&cmd_list_set]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            dynamic_cast<Null::CommandListSet&>(cmd_list_set.GetInterface()).Complete();
        });

        CHECK(cmd_list.GetState() == Rhi::CommandListState::Executing);
        REQUIRE_NOTHROW(cmd_list.WaitUntilCompleted());
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Pending);
    }

    SECTION("Get GPU Time Range")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK_NOTHROW(cmd_list.GetGpuTimeRange(true) == Data::TimeRange{});
        CHECK_NOTHROW(cmd_list.GetGpuTimeRange(false) == Data::TimeRange{});
    }
}
