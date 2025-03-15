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

FILE: Tests/Graphics/RHI/ResourceBarriersTest.cpp
Unit-tests of the RHI ResourceBarriers

******************************************************************************/

#include "RhiSettings.hpp"
#include "RhiTestHelpers.hpp"

#include <Methane/Graphics/RHI/ResourceBarriers.h>
#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/Buffer.h>
#include <Methane/Graphics/RHI/Texture.h>
#include <Methane/Data/AppShadersProvider.h>

#include <memory>
#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;
static const Platform::AppEnvironment test_app_env{ nullptr };
static const Rhi::RenderContextSettings render_context_settings = Test::GetRenderContextSettings();
static const Rhi::RenderContext render_context(test_app_env, GetTestDevice(), g_parallel_executor, render_context_settings);
static const Rhi::BufferSettings const_buffer_settings = Rhi::BufferSettings::ForConstantBuffer(42000, false, true);
static const Rhi::TextureSettings image_texture_settings = Rhi::TextureSettings::ForImage(Dimensions(640, 480), {}, PixelFormat::RGBA8, false);

TEST_CASE("RHI Resource Barrier ID", "[rhi][resource][barrier][id]")
{
    const Rhi::Buffer buffer_one = render_context.CreateBuffer(const_buffer_settings);
    const Rhi::Buffer buffer_two = render_context.CreateBuffer(const_buffer_settings);

    SECTION("Barrier Id State Transition Construction")
    {
        Rhi::ResourceBarrierId resource_barrier_id(Rhi::ResourceBarrierType::StateTransition, buffer_one.GetInterface());
        CHECK(resource_barrier_id.GetType() == Rhi::ResourceBarrierType::StateTransition);
        CHECK(std::addressof(resource_barrier_id.GetResource()) == std::addressof(buffer_one.GetInterface()));
    }

    SECTION("Barrier Id Owner Transition Construction")
    {
        Rhi::ResourceBarrierId resource_barrier_id(Rhi::ResourceBarrierType::OwnerTransition, buffer_two.GetInterface());
        CHECK(resource_barrier_id.GetType() == Rhi::ResourceBarrierType::OwnerTransition);
        CHECK(std::addressof(resource_barrier_id.GetResource()) == std::addressof(buffer_two.GetInterface()));
    }

    const Rhi::ResourceBarrierId buffer_one_state_transition_id(Rhi::ResourceBarrierType::StateTransition, buffer_one.GetInterface());
    const Rhi::ResourceBarrierId buffer_one_owner_transition_id(Rhi::ResourceBarrierType::OwnerTransition, buffer_one.GetInterface());
    const Rhi::ResourceBarrierId buffer_two_state_transition_id(Rhi::ResourceBarrierType::StateTransition, buffer_two.GetInterface());
    const Rhi::ResourceBarrierId buffer_two_owner_transition_id(Rhi::ResourceBarrierType::OwnerTransition, buffer_two.GetInterface());

    SECTION("Barrier Id equality")
    {
        Rhi::ResourceBarrierId buffer_one_state_transition_2(Rhi::ResourceBarrierType::StateTransition, buffer_one.GetInterface());
        CHECK(buffer_one_state_transition_id == buffer_one_state_transition_2);
        CHECK(buffer_one_state_transition_id <= buffer_one_state_transition_2);
        CHECK(buffer_one_state_transition_id >= buffer_one_state_transition_2);
    }

    SECTION("Barrier Id non-equality by type")
    {
        CHECK(buffer_one_state_transition_id != buffer_one_owner_transition_id);
        CHECK_FALSE(buffer_one_state_transition_id == buffer_one_owner_transition_id);
    }

    SECTION("Barrier Id non-equality by resource")
    {
        CHECK(buffer_one_state_transition_id != buffer_two_state_transition_id);
        CHECK_FALSE(buffer_one_state_transition_id == buffer_two_state_transition_id);
    }

    SECTION("Barrier Id less by type")
    {
        CHECK(buffer_one_state_transition_id < buffer_one_owner_transition_id);
        CHECK(buffer_one_state_transition_id <= buffer_one_owner_transition_id);
    }

    SECTION("Barrier Id less by resource")
    {
        if (buffer_one.GetInterfacePtr().get() < buffer_two.GetInterfacePtr().get())
        {
            CHECK(buffer_one_owner_transition_id < buffer_two_owner_transition_id);
            CHECK(buffer_one_owner_transition_id <= buffer_two_owner_transition_id);
        }
        else
        {
            CHECK(buffer_two_owner_transition_id <  buffer_one_owner_transition_id);
            CHECK(buffer_two_owner_transition_id <= buffer_one_owner_transition_id);
        }
    }

    SECTION("Barrier Id greater by type")
    {
        CHECK(buffer_one_owner_transition_id > buffer_one_state_transition_id);
        CHECK(buffer_one_owner_transition_id >= buffer_one_state_transition_id);
    }

    SECTION("Barrier Id greater by resource")
    {
        if (buffer_one.GetInterfacePtr().get() < buffer_two.GetInterfacePtr().get())
        {
            CHECK(buffer_two_owner_transition_id >  buffer_one_owner_transition_id);
            CHECK(buffer_two_owner_transition_id >= buffer_one_owner_transition_id);
        }
        else
        {
            CHECK(buffer_one_owner_transition_id >  buffer_two_owner_transition_id);
            CHECK(buffer_one_owner_transition_id >= buffer_two_owner_transition_id);
        }
    }
}

TEST_CASE("RHI Resource State Change", "[rhi][resource][state]")
{
    SECTION("State Change Construction")
    {
        const Rhi::ResourceStateChange state_change(Rhi::ResourceState::CopyDest, Rhi::ResourceState::VertexBuffer);
        CHECK(state_change.GetStateBefore() == Rhi::ResourceState::CopyDest);
        CHECK(state_change.GetStateAfter()  == Rhi::ResourceState::VertexBuffer);
    }

    const Rhi::ResourceStateChange common_to_vertex_buf(Rhi::ResourceState::Common, Rhi::ResourceState::VertexBuffer);
    const Rhi::ResourceStateChange copy_dest_to_vertex_buf(Rhi::ResourceState::CopyDest, Rhi::ResourceState::VertexBuffer);
    const Rhi::ResourceStateChange copy_dest_to_index_buf(Rhi::ResourceState::CopyDest, Rhi::ResourceState::IndexBuffer);

    SECTION("State Change equality")
    {
        const Rhi::ResourceStateChange common_to_vertex_buf_2(Rhi::ResourceState::Common, Rhi::ResourceState::VertexBuffer);
        CHECK(common_to_vertex_buf == common_to_vertex_buf_2);
        CHECK(common_to_vertex_buf <= common_to_vertex_buf_2);
        CHECK(common_to_vertex_buf >= common_to_vertex_buf_2);
    }

    SECTION("State Change non-equality by before state")
    {
        CHECK(common_to_vertex_buf != copy_dest_to_vertex_buf);
        CHECK_FALSE(common_to_vertex_buf == copy_dest_to_vertex_buf);
    }

    SECTION("State Change non-equality by after state")
    {
        CHECK(copy_dest_to_vertex_buf != copy_dest_to_index_buf);
        CHECK_FALSE(copy_dest_to_vertex_buf == copy_dest_to_index_buf);
    }

    SECTION("State Change less by before state")
    {
        CHECK(common_to_vertex_buf < copy_dest_to_vertex_buf);
        CHECK(common_to_vertex_buf <= copy_dest_to_vertex_buf);
    }

    SECTION("State Change less by after state")
    {
        CHECK(copy_dest_to_vertex_buf < copy_dest_to_index_buf);
        CHECK(copy_dest_to_vertex_buf <= copy_dest_to_index_buf);
    }

    SECTION("State Change greater by before state")
    {
        CHECK(copy_dest_to_vertex_buf > common_to_vertex_buf);
        CHECK(copy_dest_to_vertex_buf >= common_to_vertex_buf);
    }

    SECTION("State Change greater by after state")
    {
        CHECK(copy_dest_to_index_buf > copy_dest_to_vertex_buf);
        CHECK(copy_dest_to_index_buf >= copy_dest_to_vertex_buf);
    }
}

TEST_CASE("RHI Resource Owner Change", "[rhi][resource][owner]")
{
    SECTION("Owner Change Construction")
    {
        const Rhi::ResourceOwnerChange owner_change(0U, 1U);
        CHECK(owner_change.GetQueueFamilyBefore() == 0U);
        CHECK(owner_change.GetQueueFamilyAfter()  == 1U);
    }

    const Rhi::ResourceOwnerChange queue_change_0_to_1(0U, 1U);
    const Rhi::ResourceOwnerChange queue_change_0_to_2(0U, 2U);
    const Rhi::ResourceOwnerChange queue_change_1_to_2(1U, 2U);

    SECTION("Owner Change equality")
    {
        const Rhi::ResourceOwnerChange queue_change_1_to_2_copy(1U, 2U);
        CHECK(queue_change_1_to_2 == queue_change_1_to_2_copy);
        CHECK(queue_change_1_to_2 <= queue_change_1_to_2_copy);
        CHECK(queue_change_1_to_2 >= queue_change_1_to_2_copy);
    }

    SECTION("Owner Change non-equality by before queue family")
    {
        CHECK(queue_change_0_to_2 != queue_change_1_to_2);
        CHECK_FALSE(queue_change_0_to_2 == queue_change_1_to_2);
    }

    SECTION("Owner Change non-equality by after queue family")
    {
        CHECK(queue_change_0_to_1 != queue_change_0_to_2);
        CHECK_FALSE(queue_change_0_to_1 == queue_change_0_to_2);
    }

    SECTION("Owner Change less by before queue family")
    {
        CHECK(queue_change_0_to_2 < queue_change_1_to_2);
        CHECK(queue_change_0_to_2 <= queue_change_1_to_2);
    }

    SECTION("Owner Change less by after queue family")
    {
        CHECK(queue_change_0_to_1 < queue_change_0_to_2);
        CHECK(queue_change_0_to_1 <= queue_change_0_to_2);
    }

    SECTION("Owner Change greater by before queue family")
    {
        CHECK(queue_change_1_to_2 > queue_change_0_to_2);
        CHECK(queue_change_1_to_2 >= queue_change_0_to_2);
    }

    SECTION("Owner Change greater by after queue family")
    {
        CHECK(queue_change_0_to_2 > queue_change_0_to_1);
        CHECK(queue_change_0_to_2 >= queue_change_0_to_1);
    }
}

TEST_CASE("RHI Resource Barrier", "[rhi][resource][barrier]")
{
    const Rhi::Buffer buffer = render_context.CreateBuffer(const_buffer_settings);
    const Rhi::ResourceStateChange state_change(Rhi::ResourceState::CopyDest, Rhi::ResourceState::VertexBuffer);
    const Rhi::ResourceOwnerChange owner_change(0U, 1U);

    SECTION("State Transition Barrier Construction from States")
    {
        const Rhi::ResourceBarrier barrier(buffer.GetInterface(), Rhi::ResourceState::CopyDest, Rhi::ResourceState::VertexBuffer);
        CHECK(std::addressof(barrier.GetId().GetResource()) == std::addressof(buffer.GetInterface()));
        CHECK(barrier.GetId().GetType() == Rhi::ResourceBarrierType::StateTransition);
        CHECK(barrier.GetStateChange().GetStateBefore() == Rhi::ResourceState::CopyDest);
        CHECK(barrier.GetStateChange().GetStateAfter() == Rhi::ResourceState::VertexBuffer);
    }

    SECTION("State Transition Barrier Construction from State Change")
    {
        const Rhi::ResourceBarrier barrier(buffer.GetInterface(), state_change);
        CHECK(std::addressof(barrier.GetId().GetResource()) == std::addressof(buffer.GetInterface()));
        CHECK(barrier.GetStateChange() == state_change);
    }

    SECTION("Owner Transition Barrier Construction from Owner Change")
    {
        const Rhi::ResourceBarrier barrier(buffer.GetInterface(), 0U, 1U);
        CHECK(std::addressof(barrier.GetId().GetResource()) == std::addressof(buffer.GetInterface()));
        CHECK(barrier.GetId().GetType() == Rhi::ResourceBarrierType::OwnerTransition);
        CHECK(barrier.GetOwnerChange().GetQueueFamilyBefore() == 0U);
        CHECK(barrier.GetOwnerChange().GetQueueFamilyAfter() == 1U);
    }

    SECTION("Owner Transition Barrier Construction from Owner Change")
    {
        const Rhi::ResourceBarrier barrier(buffer.GetInterface(), owner_change);
        CHECK(std::addressof(barrier.GetId().GetResource()) == std::addressof(buffer.GetInterface()));
        CHECK(barrier.GetOwnerChange() == owner_change);
    }

    const Rhi::ResourceBarrier buffer_state_transition(buffer.GetInterface(), state_change);
    const Rhi::ResourceBarrier buffer_owner_transition(buffer.GetInterface(), owner_change);

    SECTION("Barrier Equality")
    {
        const Rhi::ResourceBarrier buffer_state_transition_2(buffer.GetInterface(), state_change);
        CHECK(buffer_state_transition == buffer_state_transition_2);
        CHECK(buffer_state_transition <= buffer_state_transition_2);
        CHECK(buffer_state_transition >= buffer_state_transition_2);
        CHECK_FALSE(buffer_state_transition != buffer_state_transition_2);
    }

    SECTION("Barrier Non-Equality by Resource")
    {
        const Rhi::Buffer another_buffer = render_context.CreateBuffer(const_buffer_settings);
        const Rhi::ResourceBarrier another_buffer_state_transition(another_buffer.GetInterface(), state_change);
        CHECK(buffer_state_transition != another_buffer_state_transition);
        CHECK_FALSE(buffer_state_transition == another_buffer_state_transition);
    }

    SECTION("Can Not Compare Barriers of Different Type")
    {
        CHECK_THROWS_AS(buffer_state_transition <  buffer_owner_transition, ArgumentException);
        CHECK_THROWS_AS(buffer_state_transition <= buffer_owner_transition, ArgumentException);
        CHECK_THROWS_AS(buffer_state_transition == buffer_owner_transition, ArgumentException);
        CHECK_THROWS_AS(buffer_state_transition >= buffer_owner_transition, ArgumentException);
        CHECK_THROWS_AS(buffer_state_transition >  buffer_owner_transition, ArgumentException);
    }

    SECTION("Apply State Transition to Resource from Matching State")
    {
        CHECK(buffer.SetState(buffer_state_transition.GetStateChange().GetStateBefore()));
        buffer_state_transition.ApplyTransition();
        CHECK(buffer.GetState() == buffer_state_transition.GetStateChange().GetStateAfter());
    }

    SECTION("Can not Apply State Transition to Resource from Wrong State")
    {
        CHECK(buffer.SetState(Rhi::ResourceState::CopySource));
        CHECK_THROWS_AS(buffer_state_transition.ApplyTransition(), ArgumentException);
    }

    SECTION("Apply Owner Transition to Resource from Matching Queue Family")
    {
        CHECK(buffer.SetOwnerQueueFamily(buffer_owner_transition.GetOwnerChange().GetQueueFamilyBefore()));
        buffer_owner_transition.ApplyTransition();
        CHECK(buffer.GetOwnerQueueFamily() == buffer_owner_transition.GetOwnerChange().GetQueueFamilyAfter());
    }

    SECTION("Can not Apply Owner Transition to Resource from Wrong Queue Family")
    {
        CHECK(buffer.SetOwnerQueueFamily(42U));
        CHECK_THROWS_AS(buffer_state_transition.ApplyTransition(), ArgumentException);
    }

    SECTION("Can not Apply Owner Transition to Resource from Undefined Queue Family")
    {
        CHECK(!buffer.GetOwnerQueueFamily().has_value());
        CHECK_THROWS_AS(buffer_state_transition.ApplyTransition(), ArgumentException);
    }
}