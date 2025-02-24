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
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/ComputeCommandList.h>
#include <Methane/Graphics/RHI/Device.h>

#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

TEST_CASE("RHI Command List Set Functions", "[rhi][list][set]")
{
    const Rhi::ComputeContext compute_context = Rhi::ComputeContext(GetTestDevice(), g_parallel_executor, {});
    const Rhi::CommandQueue compute_cmd_queue = compute_context.CreateCommandQueue(Rhi::CommandListType::Compute);
    const Rhi::ComputeCommandList command_list_one = compute_cmd_queue.CreateComputeCommandList();
    const Rhi::ComputeCommandList command_list_two = compute_cmd_queue.CreateComputeCommandList();
    const Rhi::ComputeCommandList command_list_three = compute_cmd_queue.CreateComputeCommandList();
    const Refs<Rhi::ICommandList> command_list_refs = {
        command_list_one.GetInterface(),
        command_list_two.GetInterface(),
        command_list_three.GetInterface()
    };

    SECTION("Command List Set Construction")
    {
        Rhi::CommandListSet command_list_set;
        REQUIRE_NOTHROW(command_list_set = Rhi::CommandListSet(command_list_refs));
        REQUIRE(command_list_set.IsInitialized());
        CHECK(command_list_set.GetCount() == command_list_refs.size());
        CHECK(command_list_set.GetFrameIndex() == std::nullopt);
    }

    const Data::Index command_frame_index = 2U;
    const Rhi::CommandListSet command_list_set = Rhi::CommandListSet(command_list_refs, command_frame_index);

    SECTION("Get Frame Index")
    {
        CHECK(command_list_set.GetFrameIndex() == command_frame_index);
    }

    SECTION("Get Command Lists Count")
    {
        CHECK(command_list_set.GetCount() == command_list_refs.size());
    }

    SECTION("Get Command List References")
    {
        Data::Index command_list_index = 0;
        for(const Ref<Rhi::ICommandList> command_list_ref : command_list_set.GetRefs())
        {
            CHECK(std::addressof(command_list_ref.get()) ==
                  std::addressof(command_list_refs[command_list_index].get()));
            command_list_index++;
        }
    }

    SECTION("Indexed access operator")
    {
        for(Data::Index command_list_index = 0; command_list_index < command_list_refs.size(); ++command_list_index)
        {
            CHECK(std::addressof(command_list_set[command_list_index]) ==
                  std::addressof(command_list_refs[command_list_index].get()));
        }
    }
}