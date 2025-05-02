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

FILE: Tests/Graphics/RHI/RenderStateTest.cpp
Unit-tests of the RHI RenderState

******************************************************************************/

#include "RhiTestHelpers.hpp"
#include "RhiSettings.hpp"

#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/RenderState.h>
#include <Methane/Graphics/RHI/Program.h>
#include <Methane/Graphics/RHI/Shader.h>
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
static const Rhi::RenderPattern render_pattern(render_context, render_pattern_settings);

static const Rhi::RenderStateSettingsImpl render_state_settings = Test::GetRenderStateSettings(render_context, render_pattern);

TEST_CASE("RHI Render State Functions", "[rhi][render][state]")
{
    SECTION("Render State Construction")
    {
        Rhi::RenderState render_state;
        REQUIRE_NOTHROW(render_state = render_context.CreateRenderState(render_state_settings));
        REQUIRE(render_state.IsInitialized());
        CHECK(render_state.GetInterfacePtr());
        CHECK(render_state.GetSettings() == Rhi::RenderStateSettingsImpl::Convert(render_state_settings));
    }

    SECTION("Object Destroyed Callback")
    {
        auto render_state_ptr = std::make_unique<Rhi::RenderState>(render_context, render_state_settings);
        ObjectCallbackTester object_callback_tester(*render_state_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        render_state_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    const Rhi::RenderState render_state = render_context.CreateRenderState(render_state_settings);

    SECTION("Object Name Setup")
    {
        CHECK(render_state.SetName("My Render State"));
        CHECK(render_state.GetName() == "My Render State");
    }

    SECTION("Object Name Change Callback")
    {
        CHECK(render_state.SetName("My Render State"));
        ObjectCallbackTester object_callback_tester(render_state);
        CHECK(render_state.SetName("Our Render State"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Render State");
        CHECK(object_callback_tester.GetOldObjectName() == "My Render State");
    }

    SECTION("Object Name Set Unchanged")
    {
        CHECK(render_state.SetName("My Render State"));
        ObjectCallbackTester object_callback_tester(render_state);
        CHECK_FALSE(render_state.SetName("My Render State"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Add to Objects Registry")
    {
        render_state.SetName("Render State");
        Rhi::ObjectRegistry registry = render_context.GetObjectRegistry();
        registry.AddGraphicsObject(render_state);
        const auto registered_state = registry.GetGraphicsObject<Rhi::RenderState>("Render State");
        REQUIRE(registered_state.IsInitialized());
        CHECK(&registered_state.GetInterface() == &render_state.GetInterface());
    }

    SECTION("Reset with Settings Impl")
    {
        Rhi::RenderStateSettingsImpl new_render_state_settings = render_state_settings;
        new_render_state_settings.program = render_context.CreateProgram({
            { { Rhi::ShaderType::Vertex, { Data::ShaderProvider::Get(), { "Render", "New" } } } },
        });
        REQUIRE_NOTHROW(render_state.Reset(new_render_state_settings));
        CHECK(render_state.GetProgram().GetInterfacePtr().get() == new_render_state_settings.program.GetInterfacePtr().get());
    }

    SECTION("Reset with Settings")
    {
        Rhi::RenderStateSettingsImpl new_render_state_settings = render_state_settings;
        new_render_state_settings.program = render_context.CreateProgram({
            { { Rhi::ShaderType::Vertex, { Data::ShaderProvider::Get(), { "Render", "New" } } } },
        });
        REQUIRE_NOTHROW(render_state.Reset(Rhi::RenderStateSettingsImpl::Convert(new_render_state_settings)));
        CHECK(render_state.GetProgram().GetInterfacePtr().get() == new_render_state_settings.program.GetInterfacePtr().get());
    }

    SECTION("Get Program")
    {
        CHECK(render_state.GetProgram().GetInterfacePtr().get() == render_state_settings.program.GetInterfacePtr().get());
    }

    SECTION("Get Render Pattern")
    {
        CHECK(render_state.GetRenderPattern().GetInterfacePtr().get() == render_state_settings.render_pattern.GetInterfacePtr().get());
    }
}
