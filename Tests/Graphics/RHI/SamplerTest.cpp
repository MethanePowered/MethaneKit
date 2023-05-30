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

FILE: Tests/Graphics/RHI/SamplerTest.cpp
Unit-tests of the RHI Sampler

******************************************************************************/

#include "RhiTestHelpers.hpp"

#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/Sampler.h>
#include <Methane/Graphics/RHI/ResourceBarriers.h>
#include <Methane/Graphics/RHI/CommandKit.h>
#include <Methane/Graphics/RHI/CommandQueue.h>

#include <memory>
#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

TEST_CASE("RHI Sampler Functions", "[rhi][sampler][resource]")
{
    const Rhi::ComputeContext compute_context = Rhi::ComputeContext(GetTestDevice(), g_parallel_executor, {});
    const Rhi::SamplerSettings sampler_settings{
        rhi::SamplerFilter  { rhi::SamplerFilter::MinMag::Linear },
        rhi::SamplerAddress { rhi::SamplerAddress::Mode::ClampToEdge },
        rhi::SamplerLevelOfDetail{ 0.5f, 0.f, 1.f },
        2U, rhi::SamplerBorderColor::OpaqueBlack,
        Compare::GreaterEqual
    };

    SECTION("Constant Sampler Construction")
    {
        Rhi::Sampler sampler;
        REQUIRE_NOTHROW(sampler = compute_context.CreateSampler(sampler_settings));
        REQUIRE(sampler.IsInitialized());
        CHECK(sampler.GetInterfacePtr());
        CHECK(sampler.GetResourceType() == Rhi::ResourceType::Sampler);
        CHECK(sampler.GetSettings() == sampler_settings);
        CHECK(std::addressof(sampler.GetContext()) == compute_context.GetInterfacePtr().get());
    }

    SECTION("Object Destroyed Callback")
    {
        auto sampler_ptr = std::make_unique<Rhi::Sampler>(compute_context, sampler_settings);
        ObjectCallbackTester object_callback_tester(*sampler_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        sampler_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    SECTION("Resource Released Callback")
    {
        auto sampler_ptr = std::make_unique<Rhi::Sampler>(compute_context, sampler_settings);
        ResourceCallbackTester resource_callback_tester(*sampler_ptr);
        CHECK_FALSE(resource_callback_tester.IsResourceReleased());
        sampler_ptr.reset();
        CHECK(resource_callback_tester.IsResourceReleased());
    }

    const Rhi::Sampler sampler = compute_context.CreateSampler(sampler_settings);

    SECTION("Object Name Setup")
    {
        CHECK(sampler.SetName("My Sampler"));
        CHECK(sampler.GetName() == "My Sampler");
    }

    SECTION("Object Name Change Callback")
    {
        CHECK(sampler.SetName("My Sampler"));
        ObjectCallbackTester object_callback_tester(sampler);
        CHECK(sampler.SetName("Our Sampler"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Sampler");
        CHECK(object_callback_tester.GetOldObjectName() == "My Sampler");
    }

    SECTION("Object Name Set Unchanged")
    {
        CHECK(sampler.SetName("My Sampler"));
        ObjectCallbackTester object_callback_tester(sampler);
        CHECK_FALSE(sampler.SetName("My Sampler"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Set State")
    {
        CHECK(sampler.GetState() == Rhi::ResourceState::Undefined);
        CHECK(sampler.SetState(Rhi::ResourceState::ShaderResource));
        CHECK(sampler.GetState() == Rhi::ResourceState::ShaderResource);
    }

    SECTION("Set State with Barriers")
    {
        Rhi::ResourceBarriers resource_barriers;
        CHECK(sampler.SetState(Rhi::ResourceState::CopyDest));
        CHECK(sampler.SetState(Rhi::ResourceState::ShaderResource, resource_barriers));
        CHECK(sampler.GetState() == Rhi::ResourceState::ShaderResource);
        CHECK(resource_barriers.HasStateTransition(sampler.GetInterface(),
                                                   Rhi::ResourceState::CopyDest,
                                                   Rhi::ResourceState::ShaderResource));
    }

    SECTION("Set Owner Queue Family")
    {
        CHECK_FALSE(sampler.GetOwnerQueueFamily().has_value());
        CHECK(sampler.SetOwnerQueueFamily(1U));
        REQUIRE(sampler.GetOwnerQueueFamily().has_value());
        CHECK(sampler.GetOwnerQueueFamily().value() == 1U);
    }

    SECTION("Set Owner Queue Family with Barriers")
    {
        Rhi::ResourceBarriers resource_barriers;
        CHECK(sampler.SetOwnerQueueFamily(0U));
        CHECK(sampler.SetOwnerQueueFamily(1U, resource_barriers));
        REQUIRE(sampler.GetOwnerQueueFamily().has_value());
        CHECK(sampler.GetOwnerQueueFamily().value() == 1U);
        CHECK(resource_barriers.HasOwnerTransition(sampler.GetInterface(), 0U, 1U));
    }

    SECTION("Restore Descriptor Views")
    {
        auto sampler_ptr = std::make_unique<Rhi::Sampler>(compute_context, sampler_settings);
        const Rhi::Sampler::DescriptorByViewId descriptor_by_view_id = sampler_ptr->GetDescriptorByViewId();
        sampler_ptr = std::make_unique<Rhi::Sampler>(compute_context, sampler_settings);
        CHECK_NOTHROW(sampler_ptr->RestoreDescriptorViews(descriptor_by_view_id));
    }
}
