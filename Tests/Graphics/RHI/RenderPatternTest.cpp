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

FILE: Tests/Graphics/RHI/RenderPatternTest.cpp
Unit-tests of the RHI RenderPattern

******************************************************************************/

#include "RhiTestHelpers.hpp"
#include "RhiSettings.hpp"

#include <Methane/Data/AppShadersProvider.h>
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

TEST_CASE("RHI Render Pattern Functions", "[rhi][render][pattern]")
{
    SECTION("Render Pattern Construction")
    {
        Rhi::RenderPattern render_pattern;
        REQUIRE_NOTHROW(render_pattern = render_context.CreateRenderPattern(render_pattern_settings));
        REQUIRE(render_pattern.IsInitialized());
        CHECK(render_pattern.GetInterfacePtr());
        CHECK(render_pattern.GetSettings() == render_pattern_settings);
    }

    SECTION("Object Destroyed Callback")
    {
        auto render_pattern_ptr = std::make_unique<Rhi::RenderPattern>(render_context, render_pattern_settings);
        ObjectCallbackTester object_callback_tester(*render_pattern_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        render_pattern_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    const Rhi::RenderPattern render_pattern = render_context.CreateRenderPattern(render_pattern_settings);

    SECTION("Object Name Setup")
    {
        CHECK(render_pattern.SetName("My Render Pattern"));
        CHECK(render_pattern.GetName() == "My Render Pattern");
    }

    SECTION("Object Name Change Callback")
    {
        CHECK(render_pattern.SetName("My Render Pattern"));
        ObjectCallbackTester object_callback_tester(render_pattern);
        CHECK(render_pattern.SetName("Our Render Pattern"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Render Pattern");
        CHECK(object_callback_tester.GetOldObjectName() == "My Render Pattern");
    }

    SECTION("Object Name Set Unchanged")
    {
        CHECK(render_pattern.SetName("My Render Pattern"));
        ObjectCallbackTester object_callback_tester(render_pattern);
        CHECK_FALSE(render_pattern.SetName("My Render Pattern"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Add to Objects Registry")
    {
        render_pattern.SetName("Render Pattern");
        Rhi::ObjectRegistry registry = render_context.GetObjectRegistry();
        registry.AddGraphicsObject(render_pattern);
        const auto registered_pattern = registry.GetGraphicsObject<Rhi::RenderPattern>("Render Pattern");
        REQUIRE(registered_pattern.IsInitialized());
        CHECK(&registered_pattern.GetInterface() == &render_pattern.GetInterface());
    }

    SECTION("Get Settings")
    {
        CHECK(render_pattern.GetSettings() == render_pattern_settings);
    }

    SECTION("Get Render Context")
    {
        CHECK(render_pattern.GetRenderContext().GetInterfacePtr().get() ==
              render_context.GetInterfacePtr().get());
    }

    SECTION("Get Attachment Count")
    {
        Data::Size attachments_count = render_pattern_settings.color_attachments.size();
        if (render_pattern_settings.depth_attachment)
        {
            attachments_count++;
        }
        if (render_pattern_settings.stencil_attachment)
        {
            attachments_count++;
        }
        CHECK(render_pattern.GetAttachmentCount() == attachments_count);
    }

    SECTION("Get Attachment Formats")
    {
        const AttachmentFormats attachment_formats = render_pattern.GetAttachmentFormats();
        CHECK(attachment_formats.colors.size() == render_pattern_settings.color_attachments.size());
        CHECK(attachment_formats.depth == (render_pattern_settings.depth_attachment
                                        ? render_pattern_settings.depth_attachment->format
                                        : PixelFormat::Unknown));
        CHECK(attachment_formats.stencil == (render_pattern_settings.stencil_attachment
                                          ? render_pattern_settings.stencil_attachment->format
                                          : PixelFormat::Unknown));
    }
}

TEST_CASE("RHI Render Pattern Factory", "[rhi][render][pattern][factory]")
{
    const Rhi::RenderPattern render_pattern = render_context.CreateRenderPattern(render_pattern_settings);

    SECTION("Can Create Render Pass")
    {
        Rhi::RenderPass render_pass;
        Test::RenderPassResources resources = Test::GetRenderPassResources(render_pattern);
        REQUIRE_NOTHROW(render_pass = render_pattern.CreateRenderPass(resources.settings));
        REQUIRE(render_pass.IsInitialized());
        CHECK(render_pass.GetSettings() == resources.settings);
    }
}
