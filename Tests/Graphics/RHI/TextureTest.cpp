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

FILE: Tests/Graphics/RHI/TextureTest.cpp
Unit-tests of the RHI Texture

******************************************************************************/

#include "Methane/Graphics/RHI/ResourceView.h"
#include "RhiTestHelpers.hpp"

#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/Texture.h>
#include <Methane/Graphics/RHI/ResourceBarriers.h>
#include <Methane/Graphics/RHI/CommandKit.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/ObjectRegistry.h>

#include <memory>
#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

TEST_CASE("RHI Texture Functions", "[rhi][texture][resource]")
{
    const Rhi::ComputeContext compute_context = Rhi::ComputeContext(GetTestDevice(), g_parallel_executor, {});
    const Rhi::TextureSettings image_texture_settings = Rhi::TextureSettings::ForImage(Dimensions(640, 480), {}, PixelFormat::RGBA8, false);

    SECTION("Constant Texture Construction")
    {
        Rhi::Texture texture;
        REQUIRE_NOTHROW(texture = compute_context.CreateTexture(image_texture_settings));
        REQUIRE(texture.IsInitialized());
        CHECK(texture.GetInterfacePtr());
        CHECK(texture.GetResourceType() == Rhi::ResourceType::Texture);
        CHECK(texture.GetSettings() == image_texture_settings);
        CHECK(texture.GetUsage() == image_texture_settings.usage_mask);
        CHECK(std::addressof(texture.GetContext()) == compute_context.GetInterfacePtr().get());
    }

    SECTION("Object Destroyed Callback")
    {
        auto texture_ptr = std::make_unique<Rhi::Texture>(compute_context, image_texture_settings);
        ObjectCallbackTester object_callback_tester(*texture_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        texture_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    SECTION("Resource Released Callback")
    {
        auto texture_ptr = std::make_unique<Rhi::Texture>(compute_context, image_texture_settings);
        ResourceCallbackTester resource_callback_tester(*texture_ptr);
        CHECK_FALSE(resource_callback_tester.IsResourceReleased());
        texture_ptr.reset();
        CHECK(resource_callback_tester.IsResourceReleased());
    }

    const Rhi::Texture texture = compute_context.CreateTexture(image_texture_settings);

    SECTION("Object Name Setup")
    {
        CHECK(texture.SetName("My Texture"));
        CHECK(texture.GetName() == "My Texture");
    }

    SECTION("Object Name Change Callback")
    {
        CHECK(texture.SetName("My Texture"));
        ObjectCallbackTester object_callback_tester(texture);
        CHECK(texture.SetName("Our Texture"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Texture");
        CHECK(object_callback_tester.GetOldObjectName() == "My Texture");
    }

    SECTION("Object Name Set Unchanged")
    {
        CHECK(texture.SetName("My Texture"));
        ObjectCallbackTester object_callback_tester(texture);
        CHECK_FALSE(texture.SetName("My Texture"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Add to Objects Registry")
    {
        texture.SetName("My Texture");
        Rhi::ObjectRegistry registry = compute_context.GetObjectRegistry();
        registry.AddGraphicsObject(texture);
        const auto registered_texture = registry.GetGraphicsObject<Rhi::Texture>("My Texture");
        REQUIRE(registered_texture.IsInitialized());
        CHECK(&registered_texture.GetInterface() == &texture.GetInterface());
    }

    SECTION("Set State")
    {
        CHECK(texture.GetState() == Rhi::ResourceState::Undefined);
        CHECK(texture.SetState(Rhi::ResourceState::ShaderResource));
        CHECK(texture.GetState() == Rhi::ResourceState::ShaderResource);
    }

    SECTION("Set State with Barriers")
    {
        Rhi::ResourceBarriers resource_barriers;
        CHECK(texture.SetState(Rhi::ResourceState::CopyDest));
        CHECK(texture.SetState(Rhi::ResourceState::ShaderResource, resource_barriers));
        CHECK(texture.GetState() == Rhi::ResourceState::ShaderResource);
        CHECK(resource_barriers.HasStateTransition(texture.GetInterface(),
                                                   Rhi::ResourceState::CopyDest,
                                                   Rhi::ResourceState::ShaderResource));
    }

    SECTION("Set Owner Queue Family")
    {
        CHECK_FALSE(texture.GetOwnerQueueFamily().has_value());
        CHECK(texture.SetOwnerQueueFamily(1U));
        REQUIRE(texture.GetOwnerQueueFamily().has_value());
        CHECK(texture.GetOwnerQueueFamily().value() == 1U);
    }

    SECTION("Set Owner Queue Family with Barriers")
    {
        Rhi::ResourceBarriers resource_barriers;
        CHECK(texture.SetOwnerQueueFamily(0U));
        CHECK(texture.SetOwnerQueueFamily(1U, resource_barriers));
        REQUIRE(texture.GetOwnerQueueFamily().has_value());
        CHECK(texture.GetOwnerQueueFamily().value() == 1U);
        CHECK(resource_barriers.HasOwnerTransition(texture.GetInterface(), 0U, 1U));
    }

    SECTION("Restore Descriptor Views")
    {
        auto texture_ptr = std::make_unique<Rhi::Texture>(compute_context, image_texture_settings);
        const Rhi::Texture::DescriptorByViewId descriptor_by_view_id = texture_ptr->GetDescriptorByViewId();
        texture_ptr = std::make_unique<Rhi::Texture>(compute_context, image_texture_settings);
        CHECK_NOTHROW(texture_ptr->RestoreDescriptorViews(descriptor_by_view_id));
    }

    SECTION("Get Data Size")
    {
        CHECK(texture.GetDataSize(Data::MemoryState::Reserved) == 1228800U);
        CHECK(texture.GetDataSize(Data::MemoryState::Initialized) == 0U);
    }

    SECTION("Get SubResource Count and Data Size")
    {
        CHECK(texture.GetSubresourceCount() == Rhi::SubResourceCount());
        CHECK(texture.GetSubResourceDataSize(Rhi::SubResourceIndex()) == 1228800U);
    }

    SECTION("Set Data")
    {
        std::vector<std::byte> test_data(256, std::byte(8));
        REQUIRE_NOTHROW(texture.SetData(compute_context.GetComputeCommandKit().GetQueue(), {
            {
                reinterpret_cast<Data::ConstRawPtr>(test_data.data()), // NOSONAR
                static_cast<Data::Size>(test_data.size())
            }
        }));
        CHECK(texture.GetDataSize(Data::MemoryState::Initialized) == 256U);
    }

    SECTION("Get Data")
    {
        CHECK_NOTHROW(texture.GetData(compute_context.GetComputeCommandKit().GetQueue(),
                                      Rhi::SubResource::Index{}, Rhi::BytesRangeOpt{}));
    }
}
