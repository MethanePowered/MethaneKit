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

FILE: Tests/Graphics/RHI/ObjectRegistryTest.cpp
Unit-tests of the RHI ObjectRegistry.

******************************************************************************/

#include "RhiTestHelpers.hpp"

#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/ObjectRegistry.h>
#include <Methane/Graphics/RHI/Buffer.h>

#include <memory>
#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

TEST_CASE("RHI Object Registry Functions", "[rhi][object-registry]")
{
    const Rhi::ComputeContext compute_context    = Rhi::ComputeContext(GetTestDevice(), g_parallel_executor, {});
    Rhi::ObjectRegistry       object_registry    = compute_context.GetObjectRegistry();
    Rhi::Buffer             constant_buffer_one  = compute_context.CreateBuffer(Rhi::BufferSettings::ForConstantBuffer(42000, false, true));
    Rhi::Buffer             constant_buffer_two  = compute_context.CreateBuffer(Rhi::BufferSettings::ForConstantBuffer(7300, true, false));
    Rhi::Buffer             constant_buffer_thr  = compute_context.CreateBuffer(Rhi::BufferSettings::ForConstantBuffer(850, true, true));

    CHECK(constant_buffer_one.SetName("Constant Buffer 1"));
    CHECK(constant_buffer_two.SetName("Constant Buffer 2"));
    CHECK(constant_buffer_thr.SetName("Constant Buffer 3"));

    SECTION("Add Multiple Objects to Registry")
    {
        REQUIRE_NOTHROW(object_registry.AddGraphicsObject(constant_buffer_one));
        REQUIRE_NOTHROW(object_registry.AddGraphicsObject(constant_buffer_two));
        REQUIRE_NOTHROW(object_registry.AddGraphicsObject(constant_buffer_thr));
        CHECK(object_registry.HasGraphicsObject("Constant Buffer 1"));
        CHECK(object_registry.HasGraphicsObject("Constant Buffer 2"));
        CHECK(object_registry.HasGraphicsObject("Constant Buffer 3"));
    }

    SECTION("Can not Add Unnamed Object")
    {
        const Rhi::Buffer buffer = compute_context.CreateBuffer(Rhi::BufferSettings::ForConstantBuffer(42000, false, true));
        CHECK_THROWS_AS(object_registry.AddGraphicsObject(buffer), ArgumentException);
    }

    SECTION("Check Non-Existing Object in Registry")
    {
        REQUIRE_NOTHROW(object_registry.AddGraphicsObject(constant_buffer_one));
        REQUIRE_NOTHROW(object_registry.AddGraphicsObject(constant_buffer_two));
        CHECK_FALSE(object_registry.HasGraphicsObject("Constant Buffer 4"));
    }

    SECTION("Get Multiple Objects from Registry")
    {
        REQUIRE_NOTHROW(object_registry.AddGraphicsObject(constant_buffer_one));
        REQUIRE_NOTHROW(object_registry.AddGraphicsObject(constant_buffer_two));
        REQUIRE_NOTHROW(object_registry.AddGraphicsObject(constant_buffer_thr));
        CHECK(object_registry.GetGraphicsObject<Rhi::Buffer>("Constant Buffer 1").IsInitialized());
        CHECK(object_registry.GetGraphicsObject<Rhi::Buffer>("Constant Buffer 2").IsInitialized());
        CHECK(object_registry.GetGraphicsObject<Rhi::Buffer>("Constant Buffer 3").IsInitialized());
    }

    SECTION("Get Non-Existing Object from Registry")
    {
        CHECK_FALSE(object_registry.GetGraphicsObject<Rhi::Buffer>("Constant Buffer 4").IsInitialized());
    }

    SECTION("Remove Objects from Registry")
    {
        REQUIRE_NOTHROW(object_registry.AddGraphicsObject(constant_buffer_one));
        REQUIRE_NOTHROW(object_registry.RemoveGraphicsObject(constant_buffer_one));
        CHECK_FALSE(object_registry.HasGraphicsObject("Constant Buffer 1"));
    }

    SECTION("Can not Remove Unnamed Object")
    {
        const Rhi::Buffer buffer = compute_context.CreateBuffer(Rhi::BufferSettings::ForConstantBuffer(42000, false, true));
        CHECK_THROWS_AS(object_registry.RemoveGraphicsObject(buffer), ArgumentException);
    }

    SECTION("Automatically Remove Destroyed Objects from Registry")
    {
        {
            const Rhi::Buffer buffer = compute_context.CreateBuffer(Rhi::BufferSettings::ForConstantBuffer(42000, false, true));
            buffer.SetName("Temporary Buffer");
            REQUIRE_NOTHROW(object_registry.AddGraphicsObject(buffer));
            CHECK(object_registry.HasGraphicsObject("Temporary Buffer"));
        }
        CHECK_FALSE(object_registry.HasGraphicsObject("Temporary Buffer"));
    }
}