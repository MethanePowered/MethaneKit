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

FILE: Tests/Graphics/RHI/FenceTest.cpp
Unit-tests of the RHI Fence

******************************************************************************/

#include "RhiTestHelpers.hpp"

#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/CommandKit.h>
#include <Methane/Graphics/RHI/Fence.h>
#include <Methane/Graphics/Null/Fence.h>

#include <memory>
#include <taskflow/taskflow.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

static uint64_t GetFenceValue(const Rhi::Fence& fence)
{
    return dynamic_cast<Null::Fence&>(fence.GetInterface()).GetValue();
}

TEST_CASE("RHI Fence Functions", "[rhi][fence]")
{
    const Rhi::ComputeContext compute_context = Rhi::ComputeContext(GetTestDevice(), g_parallel_executor, {});
    const Rhi::CommandQueue compute_cmd_queue = compute_context.CreateCommandQueue(Rhi::CommandListType::Compute);

    SECTION("Fence Construction")
    {
        Rhi::Fence fence;
        REQUIRE_NOTHROW(fence = compute_cmd_queue.CreateFence());
        REQUIRE(fence.IsInitialized());
        CHECK(fence.GetInterfacePtr());
    }

    SECTION("Object Destroyed Callback")
    {
        auto fence_ptr = std::make_unique<Rhi::Fence>(compute_cmd_queue);
        ObjectCallbackTester object_callback_tester(*fence_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        fence_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    const Rhi::Fence fence = compute_cmd_queue.CreateFence();

    SECTION("Object Name Setup")
    {
        CHECK(fence.SetName("My Fence"));
        CHECK(fence.GetName() == "My Fence");
    }

    SECTION("Object Name Change Callback")
    {
        CHECK(fence.SetName("My Fence"));
        ObjectCallbackTester object_callback_tester(fence);
        CHECK(fence.SetName("Our Fence"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Fence");
        CHECK(object_callback_tester.GetOldObjectName() == "My Fence");
    }

    SECTION("Object Name Set Unchanged")
    {
        CHECK(fence.SetName("My Fence"));
        ObjectCallbackTester object_callback_tester(fence);
        CHECK_FALSE(fence.SetName("My Fence"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Signal Fence")
    {
        const uint64_t value = GetFenceValue(fence);
        CHECK_NOTHROW(fence.Signal());
        CHECK(GetFenceValue(fence) == value + 1);
    }

    SECTION("Wait on CPU")
    {
        CHECK_NOTHROW(fence.WaitOnCpu());
    }

    SECTION("Wait on GPU")
    {
        CHECK_NOTHROW(fence.WaitOnGpu(compute_context.GetUploadCommandKit().GetQueue()));
    }

    SECTION("Can not wait on GPU with the same queue")
    {
        CHECK_THROWS(fence.WaitOnGpu(compute_cmd_queue));
    }

    SECTION("Flush on CPU")
    {
        const uint64_t value = GetFenceValue(fence);
        CHECK_NOTHROW(fence.FlushOnCpu());
        CHECK(GetFenceValue(fence) == value + 1);
    }

    SECTION("Flush on GPU")
    {
        const uint64_t value = GetFenceValue(fence);
        CHECK_NOTHROW(fence.FlushOnGpu(compute_context.GetUploadCommandKit().GetQueue()));
        CHECK(GetFenceValue(fence) == value + 1);
    }

    SECTION("Can not flush on GPU with the same queue")
    {
        CHECK_THROWS(fence.FlushOnGpu(compute_cmd_queue));
    }
}
