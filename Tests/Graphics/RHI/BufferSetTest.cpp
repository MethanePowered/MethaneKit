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

FILE: Tests/Graphics/RHI/BufferSetTest.cpp
Unit-tests of the RHI BufferSet

******************************************************************************/

#include "RhiTestHelpers.hpp"

#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/CommandKit.h>
#include <Methane/Graphics/RHI/Buffer.h>
#include <Methane/Graphics/RHI/BufferSet.h>
#include <Methane/Graphics/Null/BufferSet.h>

#include <memory>
#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

TEST_CASE("RHI Buffer-Set Functions", "[rhi][buffer-set]")
{
    const Rhi::ComputeContext compute_context    = Rhi::ComputeContext(GetTestDevice(), g_parallel_executor, {});
    Rhi::Buffer             constant_buffer_one  = compute_context.CreateBuffer(Rhi::BufferSettings::ForConstantBuffer(42000, false, true));
    Rhi::Buffer             constant_buffer_two  = compute_context.CreateBuffer(Rhi::BufferSettings::ForConstantBuffer(7300, true, false));
    Rhi::Buffer             constant_buffer_thr  = compute_context.CreateBuffer(Rhi::BufferSettings::ForConstantBuffer(850, true, true));
    Rhi::Buffer             vertex_buffer        = compute_context.CreateBuffer(Rhi::BufferSettings::ForVertexBuffer(850, true, true));
    const Refs<Rhi::Buffer> constant_buffer_refs { constant_buffer_one, constant_buffer_two, constant_buffer_thr };

    SECTION("Buffer-Set Construction")
    {
        Rhi::BufferSet buffer_set;
        REQUIRE_NOTHROW(buffer_set = Rhi::BufferSet(Rhi::BufferType::Constant, constant_buffer_refs));
        REQUIRE(buffer_set.IsInitialized());
        CHECK(buffer_set.GetInterfacePtr());
    }

    SECTION("Inconsistent Buffer-Set Construction Failure")
    {
        Rhi::BufferSet buffer_set;
        REQUIRE_THROWS(buffer_set = Rhi::BufferSet(Rhi::BufferType::Constant, { constant_buffer_one, constant_buffer_two, vertex_buffer }));
    }

    SECTION("Object Destroyed Callback")
    {
        auto fence_ptr = std::make_unique<Rhi::BufferSet>(Rhi::BufferType::Constant, constant_buffer_refs);
        ObjectCallbackTester object_callback_tester(*fence_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        fence_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    const Rhi::BufferSet constant_buffer_set = Rhi::BufferSet(Rhi::BufferType::Constant, constant_buffer_refs);

    SECTION("Object Name Setup")
    {
        CHECK(constant_buffer_set.SetName("My Buffer-Set"));
        CHECK(constant_buffer_set.GetName() == "My Buffer-Set");
    }

    SECTION("Object Name Change Callback")
    {
        CHECK(constant_buffer_set.SetName("My Buffer-Set"));
        ObjectCallbackTester object_callback_tester(constant_buffer_set);
        CHECK(constant_buffer_set.SetName("Our Buffer-Set"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Buffer-Set");
        CHECK(object_callback_tester.GetOldObjectName() == "My Buffer-Set");
    }

    SECTION("Object Name Set Unchanged")
    {
        CHECK(constant_buffer_set.SetName("My Buffer-Set"));
        ObjectCallbackTester object_callback_tester(constant_buffer_set);
        CHECK_FALSE(constant_buffer_set.SetName("My Buffer-Set"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Get Type of Buffer-Set")
    {
        REQUIRE(constant_buffer_set.GetType() == Rhi::BufferType::Constant);
    }

    SECTION("Get Count of Buffer-Set")
    {
        REQUIRE(constant_buffer_set.GetCount() == 3);
    }

    SECTION("Get Refs of Buffer-Set")
    {
        Rhi::BufferSet::Buffers buffers;
        CHECK_NOTHROW(buffers = constant_buffer_set.GetRefs());
        REQUIRE(buffers.size() == 3);
        CHECK(buffers.at(0).GetSettings() == constant_buffer_one.GetSettings());
        CHECK(buffers.at(1).GetSettings() == constant_buffer_two.GetSettings());
        CHECK(buffers.at(2).GetSettings() == constant_buffer_thr.GetSettings());
    }

    SECTION("Get Names of Buffer-Set")
    {
        REQUIRE_NOTHROW(constant_buffer_one.SetName("Buffer One"));
        REQUIRE_NOTHROW(constant_buffer_two.SetName("Buffer Two"));
        REQUIRE_NOTHROW(constant_buffer_thr.SetName("Buffer Three"));
        REQUIRE(constant_buffer_set.GetNames() == "'Buffer One', 'Buffer Two', 'Buffer Three'");
    }

    SECTION("Get Buffer by index from Buffer-Set")
    {
        REQUIRE(constant_buffer_set[0].GetSettings() == constant_buffer_one.GetSettings());
        REQUIRE(constant_buffer_set[1].GetSettings() == constant_buffer_two.GetSettings());
        REQUIRE(constant_buffer_set[2].GetSettings() == constant_buffer_thr.GetSettings());
        CHECK_THROWS(constant_buffer_set[3]);
    }
}
