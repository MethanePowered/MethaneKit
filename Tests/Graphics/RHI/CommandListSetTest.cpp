/******************************************************************************

Copyright 2025 Evgeny Gorodetskiy

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

FILE: Tests/Graphics/RHI/CommandListSetTest.cpp
Unit-tests of the RHI Command List Set

******************************************************************************/

#include "RhiTestHelpers.hpp"

#include <Methane/Graphics/RHI/CommandListSet.h>
#include <Methane/Graphics/Null/CommandListSet.h>
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/ComputeCommandList.h>
#include <Methane/Graphics/RHI/Device.h>

#include <ranges>
#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

TEST_CASE("RHI Command List Set Functions", "[rhi][list][set]")
{
    const Rhi::ComputeContext compute_context = Rhi::ComputeContext(GetTestDevice(), g_parallel_executor, {});
    const Rhi::CommandQueue compute_cmd_queue = compute_context.CreateCommandQueue(Rhi::CommandListType::Compute);
    const Rhi::ComputeCommandList cmd_list_one = compute_cmd_queue.CreateComputeCommandList();
    const Rhi::ComputeCommandList cmd_list_two = compute_cmd_queue.CreateComputeCommandList();
    const Rhi::ComputeCommandList cmd_list_three = compute_cmd_queue.CreateComputeCommandList();
    const Refs<Rhi::ICommandList> cmd_list_refs = {
        cmd_list_one.GetInterface(),
        cmd_list_two.GetInterface(),
        cmd_list_three.GetInterface()
    };

    SECTION("Can Construct Command List Set with Three Lists from one Queue")
    {
        Rhi::CommandListSet cmd_list_set;
        REQUIRE_NOTHROW(cmd_list_set = Rhi::CommandListSet(cmd_list_refs));
        REQUIRE(cmd_list_set.IsInitialized());
        CHECK(cmd_list_set.GetCount() == cmd_list_refs.size());
        CHECK(cmd_list_set.GetFrameIndex() == std::nullopt);
    }

    SECTION("Can not Construct Command List Set with Lists from Distinct Queues")
    {
        const Rhi::CommandQueue other_cmd_queue = compute_context.CreateCommandQueue(Rhi::CommandListType::Compute);
        const Rhi::ComputeCommandList other_cmd_list = other_cmd_queue.CreateComputeCommandList();
        const Refs<Rhi::ICommandList> other_cmd_list_refs = {
            cmd_list_one.GetInterface(),
            cmd_list_two.GetInterface(),
            other_cmd_list.GetInterface()
        };
        CHECK_THROWS_AS(Rhi::CommandListSet(other_cmd_list_refs), Methane::ArgumentException);
    }

    SECTION("Can not Construct Command List Set with Empty Lists")
    {
        const Refs<Rhi::ICommandList> empty_cmd_list_refs;
        CHECK_THROWS_AS(Rhi::CommandListSet(empty_cmd_list_refs), Methane::ArgumentException);
    }

    const Data::Index cmd_frame_index = 2U;
    const Rhi::CommandListSet cmd_list_set = Rhi::CommandListSet(cmd_list_refs, cmd_frame_index);

    SECTION("Get Frame Index")
    {
        CHECK(cmd_list_set.GetFrameIndex() == cmd_frame_index);
    }

    SECTION("Get Command Lists Count")
    {
        CHECK(cmd_list_set.GetCount() == cmd_list_refs.size());
    }

    SECTION("Get Command List References")
    {
        Data::Index cmd_list_index = 0;
        for(const Ref<Rhi::ICommandList> cmd_list_ref : cmd_list_set.GetRefs())
        {
            CHECK(std::addressof(cmd_list_ref.get()) ==
                  std::addressof(cmd_list_refs[cmd_list_index].get()));
            cmd_list_index++;
        }
    }

    SECTION("Indexed access operator")
    {
        for(Data::Index cmd_list_index = 0; cmd_list_index < cmd_list_refs.size(); ++cmd_list_index)
        {
            CHECK(std::addressof(cmd_list_set[cmd_list_index]) ==
                  std::addressof(cmd_list_refs[cmd_list_index].get()));
        }
    }

    SECTION("Can be Executed by Command Queue")
    {
        for(const Ref<Rhi::ICommandList>& cmd_list_ref : cmd_list_set.GetRefs())
        {
            REQUIRE_NOTHROW(cmd_list_ref.get().Reset());
            REQUIRE_NOTHROW(cmd_list_ref.get().Commit());
            CHECK(cmd_list_ref.get().GetState() == Rhi::CommandListState::Committed);
        }

        uint32_t completed_cmd_list_count = 0U;
        REQUIRE_NOTHROW(compute_cmd_queue.Execute(cmd_list_set,
            [&cmd_list_refs, &completed_cmd_list_count](Rhi::ICommandList& completed_cmd_list)
            {
                const auto cmd_list_ref_it = std::ranges::find_if(cmd_list_refs,
                    [&completed_cmd_list](const Ref<Rhi::ICommandList>& cmd_list_ref)
                    { return std::addressof(cmd_list_ref.get()) == std::addressof(completed_cmd_list); });
                CHECK(cmd_list_ref_it != cmd_list_refs.end());
                CHECK(completed_cmd_list.GetState() == Rhi::CommandListState::Pending);
                completed_cmd_list_count++;
            }));

        for(const Ref<Rhi::ICommandList>& cmd_list_ref : cmd_list_set.GetRefs())
        {
            CHECK(cmd_list_ref.get().GetState() == Rhi::CommandListState::Executing);
        }

        dynamic_cast<Null::CommandListSet&>(cmd_list_set.GetInterface()).Complete();
        CHECK(completed_cmd_list_count == cmd_list_refs.size());
    }
}