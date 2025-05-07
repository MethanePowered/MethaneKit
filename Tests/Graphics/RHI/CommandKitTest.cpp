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

FILE: Tests/Graphics/RHI/CommandKitTest.cpp
Unit-tests of the RHI CommandKit

******************************************************************************/

#include "RhiTestHelpers.hpp"

#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/CommandKit.h>
#include <Methane/Graphics/RHI/ComputeCommandList.h>
#include <Methane/Graphics/RHI/TransferCommandList.h>
#include <Methane/Graphics/RHI/RenderCommandList.h>
#include <Methane/Graphics/RHI/CommandListSet.h>
#include <Methane/Graphics/RHI/Fence.h>
#include <Methane/Graphics/RHI/ObjectRegistry.h>

#include <Methane/Graphics/Base/CommandList.h>

#include <memory>
#include <taskflow/taskflow.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

TEST_CASE("RHI Command Kit Functions", "[rhi][command-kit]")
{
    const auto              compute_context   = Rhi::ComputeContext(GetTestDevice(), g_parallel_executor, {});
    const Rhi::CommandQueue compute_cmd_queue = compute_context.CreateCommandQueue(Rhi::CommandListType::Compute);

    SECTION("Command Kit Construction")
    {
        Rhi::CommandKit command_kit;
        REQUIRE_NOTHROW(command_kit = compute_cmd_queue.CreateCommandKit());
        REQUIRE(command_kit.IsInitialized());
        CHECK(command_kit.GetInterfacePtr());
    }

    SECTION("Object Destroyed Callback")
    {
        auto command_kit_ptr = std::make_unique<Rhi::CommandKit>(compute_cmd_queue);
        ObjectCallbackTester object_callback_tester(*command_kit_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        command_kit_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    const Rhi::CommandKit compute_cmd_kit = compute_cmd_queue.CreateCommandKit();

    SECTION("Object Name Setup")
    {
        CHECK(compute_cmd_kit.SetName("My Command Kit"));
        CHECK(compute_cmd_kit.GetName() == "My Command Kit");
    }

    SECTION("Object Name Change Callback")
    {
        CHECK(compute_cmd_kit.SetName("My Command Kit"));
        ObjectCallbackTester object_callback_tester(compute_cmd_kit);
        CHECK(compute_cmd_kit.SetName("Our Command Kit"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Command Kit");
        CHECK(object_callback_tester.GetOldObjectName() == "My Command Kit");
    }

    SECTION("Object Name Set Unchanged")
    {
        CHECK(compute_cmd_kit.SetName("My Command Kit"));
        ObjectCallbackTester object_callback_tester(compute_cmd_kit);
        CHECK_FALSE(compute_cmd_kit.SetName("My Command Kit"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Add to Objects Registry")
    {
        compute_cmd_kit.SetName("Compute Command Kit");
        Rhi::ObjectRegistry registry = compute_context.GetObjectRegistry();
        registry.AddGraphicsObject(compute_cmd_kit);
        const auto registered_cmd_kit = registry.GetGraphicsObject<Rhi::CommandKit>("Compute Command Kit");
        REQUIRE(registered_cmd_kit.IsInitialized());
        CHECK(&registered_cmd_kit.GetInterface() == &compute_cmd_kit.GetInterface());
    }

    SECTION("Get Context of Compute Command Kit")
    {
        CHECK(compute_cmd_kit.GetContext().GetType() == Rhi::ContextType::Compute);
        CHECK(&compute_cmd_kit.GetContext() == dynamic_cast<const Rhi::IContext*>(&compute_context.GetInterface()));
    }

    SECTION("Get Queue of Compute Command Kit")
    {
        CHECK(&compute_cmd_kit.GetQueue().GetInterface() == &compute_cmd_queue.GetInterface());
    }

    SECTION("Get List Type of Compute Command Kit")
    {
        CHECK(compute_cmd_kit.GetListType() == Rhi::CommandListType::Compute);
    }

    SECTION("Can Get Compute Command List of Compute Command Kit")
    {
        const Rhi::CommandListId cmd_list_id = 0U;
        Rhi::ComputeCommandList cmd_list;
        CHECK_FALSE(compute_cmd_kit.HasList(cmd_list_id));
        REQUIRE_NOTHROW(cmd_list = compute_cmd_kit.GetComputeList(cmd_list_id));
        REQUIRE(cmd_list.IsInitialized());
        CHECK(compute_cmd_kit.HasList(cmd_list_id));
        CHECK(compute_cmd_kit.HasListWithState(Rhi::CommandListState::Pending, cmd_list_id));
    }

    SECTION("Can Get Compute Command List For Encoding of Compute Command Kit")
    {
        const Rhi::CommandListId cmd_list_id = 0U;
        Rhi::ComputeCommandList cmd_list;
        REQUIRE_NOTHROW(cmd_list = compute_cmd_kit.GetComputeListForEncoding(cmd_list_id));
        REQUIRE(cmd_list.IsInitialized());
        CHECK(compute_cmd_kit.HasListWithState(Rhi::CommandListState::Encoding, cmd_list_id));
    }

    SECTION("Can not Get Transfer Command List of Compute Command Kit")
    {
        const Rhi::CommandListId cmd_list_id = 1U;
        Rhi::TransferCommandList cmd_list;
        CHECK_FALSE(compute_cmd_kit.HasList(cmd_list_id));
        REQUIRE_THROWS(cmd_list = compute_cmd_kit.GetTransferList(cmd_list_id));
        REQUIRE_FALSE(cmd_list.IsInitialized());
        CHECK_FALSE(compute_cmd_kit.HasList(cmd_list_id));
        CHECK_FALSE(compute_cmd_kit.HasListWithState(Rhi::CommandListState::Pending, cmd_list_id));
    }

    SECTION("Can not Get Transfer Command List For Encoding of Compute Command Kit")
    {
        const Rhi::CommandListId cmd_list_id = 1U;
        Rhi::TransferCommandList cmd_list;
        CHECK_FALSE(compute_cmd_kit.HasList(cmd_list_id));
        REQUIRE_THROWS(cmd_list = compute_cmd_kit.GetTransferListForEncoding(cmd_list_id));
        REQUIRE_FALSE(cmd_list.IsInitialized());
        CHECK_FALSE(compute_cmd_kit.HasList(cmd_list_id));
        CHECK_FALSE(compute_cmd_kit.HasListWithState(Rhi::CommandListState::Encoding, cmd_list_id));
    }

    SECTION("Can not Get Render Command List of Compute Command Kit")
    {
        const Rhi::CommandListId cmd_list_id = 2U;
        Rhi::RenderCommandList cmd_list;
        CHECK_FALSE(compute_cmd_kit.HasList(cmd_list_id));
        REQUIRE_THROWS(cmd_list = compute_cmd_kit.GetRenderList(cmd_list_id));
        REQUIRE_FALSE(cmd_list.IsInitialized());
        CHECK_FALSE(compute_cmd_kit.HasList(cmd_list_id));
        CHECK_FALSE(compute_cmd_kit.HasListWithState(Rhi::CommandListState::Pending, cmd_list_id));
    }

    SECTION("Can not Get Render Command List For Encoding of Compute Command Kit")
    {
        const Rhi::CommandListId cmd_list_id = 2U;
        Rhi::RenderCommandList cmd_list;
        CHECK_FALSE(compute_cmd_kit.HasList(cmd_list_id));
        REQUIRE_THROWS(cmd_list = compute_cmd_kit.GetRenderListForEncoding(cmd_list_id));
        REQUIRE_FALSE(cmd_list.IsInitialized());
        CHECK_FALSE(compute_cmd_kit.HasList(cmd_list_id));
        CHECK_FALSE(compute_cmd_kit.HasListWithState(Rhi::CommandListState::Encoding, cmd_list_id));
    }

    constexpr std::array<Rhi::CommandListId, 2U> g_command_list_ids_0_1 = { 0U, 1U };

    SECTION("Get Command List Set of Compute Command Kit")
    {
        Rhi::CommandListSet cmd_list_set;
        REQUIRE_NOTHROW(cmd_list_set = compute_cmd_kit.GetListSet(g_command_list_ids_0_1, 2U));
        REQUIRE(cmd_list_set.IsInitialized());
        REQUIRE(cmd_list_set.GetCount() == 2);
        CHECK(cmd_list_set.GetFrameIndex() == 2U);
        CHECK(cmd_list_set[0].GetType() == Rhi::CommandListType::Compute);
        CHECK(cmd_list_set[0].GetState() == Rhi::CommandListState::Pending);
        CHECK(cmd_list_set[1].GetType() == Rhi::CommandListType::Compute);
        CHECK(cmd_list_set[1].GetState() == Rhi::CommandListState::Pending);
    }

    SECTION("Get Fences of Compute Command Kit")
    {
        Rhi::CommandListSet cmd_list_set;
        REQUIRE_NOTHROW(cmd_list_set = compute_cmd_kit.GetListSet(g_command_list_ids_0_1, 0U));
        CHECK_NOTHROW(compute_cmd_kit.GetFence(0U));
        CHECK_NOTHROW(compute_cmd_kit.GetFence(1U));
    }

    SECTION("Can Execute Non-Existing List Set")
    {
        constexpr std::array<Rhi::CommandListId, 2U> g_command_list_ids_1_2 = { 1U, 2U };
        CHECK_THROWS(compute_cmd_kit.ExecuteListSet(g_command_list_ids_1_2), 0U);
        CHECK_THROWS(compute_cmd_kit.ExecuteListSetAndWaitForCompletion(g_command_list_ids_1_2), 0U);
    }

    SECTION("Can not Execute Non-Committed List Set")
    {
        Rhi::ComputeCommandList primary_cmd_list;
        Rhi::ComputeCommandList secondary_cmd_list;
        Rhi::CommandListSet cmd_list_set;
        REQUIRE_NOTHROW(primary_cmd_list = compute_cmd_kit.GetComputeListForEncoding(0U));
        REQUIRE_NOTHROW(secondary_cmd_list = compute_cmd_kit.GetComputeListForEncoding(1U));
        CHECK_THROWS(compute_cmd_kit.ExecuteListSet(g_command_list_ids_0_1), 2U);
        CHECK_THROWS(compute_cmd_kit.ExecuteListSetAndWaitForCompletion(g_command_list_ids_0_1), 2U);
    }

    SECTION("Can Execute Committed List Set")
    {
        Rhi::ComputeCommandList primary_cmd_list;
        Rhi::ComputeCommandList secondary_cmd_list;
        Rhi::CommandListSet cmd_list_set;
        REQUIRE_NOTHROW(primary_cmd_list = compute_cmd_kit.GetComputeListForEncoding(0U));
        REQUIRE_NOTHROW(primary_cmd_list.Commit());
        REQUIRE_NOTHROW(secondary_cmd_list = compute_cmd_kit.GetComputeListForEncoding(1U));
        REQUIRE_NOTHROW(secondary_cmd_list.Commit());
        REQUIRE_NOTHROW(compute_cmd_kit.ExecuteListSet(g_command_list_ids_0_1), 2U);
        CHECK(primary_cmd_list.GetState() == Rhi::CommandListState::Executing);
        CHECK(secondary_cmd_list.GetState() == Rhi::CommandListState::Executing);
    }

    SECTION("Can Execute Committed List Set And Wait For Completion")
    {
        Rhi::ComputeCommandList primary_cmd_list;
        Rhi::ComputeCommandList secondary_cmd_list;
        Rhi::CommandListSet cmd_list_set;
        REQUIRE_NOTHROW(primary_cmd_list = compute_cmd_kit.GetComputeListForEncoding(0U));
        REQUIRE_NOTHROW(primary_cmd_list.Commit());
        REQUIRE_NOTHROW(secondary_cmd_list = compute_cmd_kit.GetComputeListForEncoding(1U));
        REQUIRE_NOTHROW(secondary_cmd_list.Commit());
        auto wait_async = g_parallel_executor.async([&compute_cmd_kit, &g_command_list_ids_0_1]()
        {
            REQUIRE_NOTHROW(compute_cmd_kit.ExecuteListSetAndWaitForCompletion(g_command_list_ids_0_1), 2U);
            for(Rhi::CommandListId cmd_list_id : g_command_list_ids_0_1)
            {
                CHECK(compute_cmd_kit.GetComputeList(cmd_list_id).GetState() == Rhi::CommandListState::Pending);
            }
        });

        uint32_t wait_count = 0;
        while (wait_count < 100 && primary_cmd_list.GetState() != Rhi::CommandListState::Executing )
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            wait_count++;
        }

        REQUIRE(primary_cmd_list.GetState() == Rhi::CommandListState::Executing);
        dynamic_cast<Base::CommandList&>(primary_cmd_list.GetInterface()).Complete();
        CHECK(primary_cmd_list.GetState() == Rhi::CommandListState::Pending);

        REQUIRE(secondary_cmd_list.GetState() == Rhi::CommandListState::Executing);
        dynamic_cast<Base::CommandList&>(secondary_cmd_list.GetInterface()).Complete();
        CHECK(secondary_cmd_list.GetState() == Rhi::CommandListState::Pending);

        wait_async.wait();
    }
}
