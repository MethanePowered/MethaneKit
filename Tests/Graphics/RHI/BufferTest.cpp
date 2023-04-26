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

FILE: Tests/Graphics/RHI/BufferTest.cpp
Unit-tests of the RHI Buffer

******************************************************************************/

#include "RhiTestHelpers.hpp"

#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/Buffer.h>
#include <Methane/Graphics/RHI/ResourceBarriers.h>

#include <memory>
#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

TEST_CASE("RHI Buffer Functions", "[rhi][buffer][resource]")
{
    const Rhi::ComputeContext compute_context = Rhi::ComputeContext(GetTestDevice(), g_parallel_executor, {});
    const Rhi::BufferSettings constant_buffer_settings = Rhi::BufferSettings::ForConstantBuffer(42000, false, true);

    SECTION("Buffer Construction")
    {
        Rhi::Buffer buffer;
        REQUIRE_NOTHROW(buffer = compute_context.CreateBuffer(constant_buffer_settings));
        REQUIRE(buffer.IsInitialized());
        CHECK(buffer.GetInterfacePtr());
        CHECK(buffer.GetSettings() == constant_buffer_settings);
    }

    SECTION("Object Destroyed Callback")
    {
        auto buffer_ptr = std::make_unique<Rhi::Buffer>(compute_context, constant_buffer_settings);
        ObjectCallbackTester object_callback_tester(*buffer_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        buffer_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    SECTION("Resource Released Callback")
    {
        auto buffer_ptr = std::make_unique<Rhi::Buffer>(compute_context, constant_buffer_settings);
        ResourceCallbackTester resource_callback_tester(*buffer_ptr);
        CHECK_FALSE(resource_callback_tester.IsResourceReleased());
        buffer_ptr.reset();
        CHECK(resource_callback_tester.IsResourceReleased());
    }

    const Rhi::Buffer buffer = compute_context.CreateBuffer(constant_buffer_settings);

    SECTION("Object Name Setup")
    {
        CHECK(buffer.SetName("My Buffer"));
        CHECK(buffer.GetName() == "My Buffer");
    }

    SECTION("Object Name Change Callback")
    {
        CHECK(buffer.SetName("My Buffer"));
        ObjectCallbackTester object_callback_tester(buffer);
        CHECK(buffer.SetName("Our Buffer"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Buffer");
        CHECK(object_callback_tester.GetOldObjectName() == "My Buffer");
    }

    SECTION("Object Name Set Unchanged")
    {
        CHECK(buffer.SetName("My Buffer"));
        ObjectCallbackTester object_callback_tester(buffer);
        CHECK_FALSE(buffer.SetName("My Buffer"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Set State")
    {
        CHECK(buffer.GetState() == Rhi::ResourceState::Undefined);
        CHECK(buffer.SetState(Rhi::ResourceState::ShaderResource));
        CHECK(buffer.GetState() == Rhi::ResourceState::ShaderResource);
    }

    SECTION("Set State with Barriers")
    {
        Rhi::ResourceBarriers resource_barriers;
        CHECK(buffer.SetState(Rhi::ResourceState::CopyDest));
        CHECK(buffer.SetState(Rhi::ResourceState::ShaderResource, resource_barriers));
        CHECK(buffer.GetState() == Rhi::ResourceState::ShaderResource);
        CHECK(resource_barriers.HasStateTransition(buffer.GetInterface(),
                                                   Rhi::ResourceState::CopyDest,
                                                   Rhi::ResourceState::ShaderResource));
    }

    SECTION("Set Owner Queue Family")
    {
        CHECK_FALSE(buffer.GetOwnerQueueFamily().has_value());
        CHECK(buffer.SetOwnerQueueFamily(1U));
        REQUIRE(buffer.GetOwnerQueueFamily().has_value());
        CHECK(buffer.GetOwnerQueueFamily().value() == 1U);
    }

    SECTION("Set Owner Queue Family with Barriers")
    {
        Rhi::ResourceBarriers resource_barriers;
        CHECK(buffer.SetOwnerQueueFamily(0U));
        CHECK(buffer.SetOwnerQueueFamily(1U, resource_barriers));
        REQUIRE(buffer.GetOwnerQueueFamily().has_value());
        CHECK(buffer.GetOwnerQueueFamily().value() == 1U);
        CHECK(resource_barriers.HasOwnerTransition(buffer.GetInterface(), 0U, 1U));
    }

    SECTION("Restore Descriptor Views")
    {
        auto buffer_ptr = std::make_unique<Rhi::Buffer>(compute_context, constant_buffer_settings);
        const Rhi::Buffer::DescriptorByViewId descriptor_by_view_id = buffer_ptr->GetDescriptorByViewId();
        buffer_ptr = std::make_unique<Rhi::Buffer>(compute_context, constant_buffer_settings);
        CHECK_NOTHROW(buffer_ptr->RestoreDescriptorViews(descriptor_by_view_id));
    }
}
