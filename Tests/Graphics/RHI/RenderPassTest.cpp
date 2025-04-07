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

FILE: Tests/Graphics/RHI/RenderPassTest.cpp
Unit-tests of the RHI RenderPass

******************************************************************************/

#include "RhiTestHelpers.hpp"
#include "RhiSettings.hpp"

#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/RenderPattern.h>
#include <Methane/Graphics/RHI/RenderPass.h>
#include <Methane/Graphics/RHI/ObjectRegistry.h>

#include <memory>
#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;
static const Platform::AppEnvironment test_app_env{ nullptr };
static const Rhi::RenderContextSettings render_context_settings = Test::GetRenderContextSettings();
static const Rhi::RenderPatternSettings render_pattern_settings = Test::GetRenderPatternSettings();
static const Rhi::RenderContext render_context(test_app_env, GetTestDevice(), g_parallel_executor, render_context_settings);

TEST_CASE("RHI Render Pass Functions", "[rhi][render][pass]")
{
    const Rhi::RenderPattern        render_pattern        = render_context.CreateRenderPattern(render_pattern_settings);
    const Test::RenderPassResources render_pass_resources = Test::GetRenderPassResources(render_pattern);

    SECTION("Render Pass Construction")
    {
        Rhi::RenderPass render_pass;
        REQUIRE_NOTHROW(render_pass = render_pattern.CreateRenderPass(render_pass_resources.settings));
        REQUIRE(render_pass.IsInitialized());
        CHECK(render_pass.GetInterfacePtr());
        CHECK(render_pass.GetSettings() == render_pass_resources.settings);
    }

    SECTION("Object Destroyed Callback")
    {
        auto render_pass_ptr = std::make_unique<Rhi::RenderPass>(render_pattern, render_pass_resources.settings);
        ObjectCallbackTester object_callback_tester(*render_pass_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        render_pass_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    const Rhi::RenderPass render_pass = render_pattern.CreateRenderPass(render_pass_resources.settings);

    SECTION("Object Name Setup")
    {
        CHECK(render_pass.SetName("My Render Pass"));
        CHECK(render_pass.GetName() == "My Render Pass");
    }

    SECTION("Object Name Change Callback")
    {
        CHECK(render_pass.SetName("My Render Pass"));
        ObjectCallbackTester object_callback_tester(render_pass);
        CHECK(render_pass.SetName("Our Render Pass"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Render Pass");
        CHECK(object_callback_tester.GetOldObjectName() == "My Render Pass");
    }

    SECTION("Object Name Set Unchanged")
    {
        CHECK(render_pass.SetName("My Render Pass"));
        ObjectCallbackTester object_callback_tester(render_pass);
        CHECK_FALSE(render_pass.SetName("My Render Pass"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Add to Objects Registry")
    {
        render_pass.SetName("Render Pass");
        Rhi::ObjectRegistry registry = render_context.GetObjectRegistry();
        registry.AddGraphicsObject(render_pass);
        const auto registered_pass = registry.GetGraphicsObject<Rhi::RenderPass>("Render Pass");
        REQUIRE(registered_pass.IsInitialized());
        CHECK(&registered_pass.GetInterface() == &render_pass.GetInterface());
    }

    SECTION("Get Settings")
    {
        CHECK(render_pass.GetSettings() == render_pass_resources.settings);
    }

    SECTION("Get Pattern")
    {
        CHECK(render_pass.GetPattern().GetInterfacePtr().get() ==
              render_pattern.GetInterfacePtr().get());
    }

    SECTION("Get Settings")
    {
        CHECK(render_pass.GetSettings() == render_pass_resources.settings);
    }

    SECTION("Update Settings")
    {
        RenderPassCallbackTester callback_tester(render_pass);
        const Test::RenderPassResources new_render_pass_resources = Test::GetRenderPassResources(render_pattern);

        CHECK_FALSE(callback_tester.IsRenderPassUpdated());
        CHECK(render_pass.GetSettings() == render_pass_resources.settings);

        CHECK_NOTHROW(render_pass.Update(new_render_pass_resources.settings));
        CHECK_FALSE(callback_tester.IsRenderPassUpdated());

        CHECK_FALSE(render_pass.GetSettings() == render_pass_resources.settings);
        CHECK(render_pass.GetSettings() == new_render_pass_resources.settings);
    }

    SECTION("Release Attachment Textures")
    {
        const Rhi::RenderPassSettings& render_pass_settings = render_pass.GetSettings();
        CHECK_FALSE(render_pass_settings.attachments.empty());
        CHECK_NOTHROW(render_pass.ReleaseAttachmentTextures());
        CHECK(render_pass_settings.attachments.empty());
    }
}
