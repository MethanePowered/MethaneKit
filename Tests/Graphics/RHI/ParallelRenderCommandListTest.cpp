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

FILE: Tests/Graphics/RHI/ParallelRenderCommandListTest.cpp
Unit-tests of the RHI Parallel Render Command List

******************************************************************************/

#include "RhiTestHelpers.hpp"
#include "RhiSettings.hpp"

#include <Methane/Data/EnumMaskUtil.hpp>
#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/ParallelRenderCommandList.h>
#include <Methane/Graphics/RHI/RenderState.h>
#include <Methane/Graphics/RHI/ViewState.h>
#include <Methane/Graphics/RHI/Program.h>
#include <Methane/Graphics/RHI/CommandListDebugGroup.h>
#include <Methane/Graphics/RHI/CommandListSet.h>
#include <Methane/Graphics/RHI/ResourceBarriers.h>
#include <Methane/Graphics/RHI/ProgramBindings.h>
#include <Methane/Graphics/RHI/Texture.h>
#include <Methane/Graphics/RHI/Buffer.h>
#include <Methane/Graphics/RHI/Sampler.h>
#include <Methane/Graphics/RHI/ObjectRegistry.h>
#include <Methane/Graphics/Base/ViewState.h>
#include <Methane/Graphics/Base/Buffer.h>
#include <Methane/Graphics/Null/CommandListSet.h>
#include <Methane/Graphics/Null/ParallelRenderCommandList.h>
#include <Methane/Graphics/Null/RenderCommandList.h>
#include <Methane/Graphics/Null/Program.h>
#include <Methane/Graphics/Null/RenderState.h>
#include <Methane/Graphics/Null/CommandListDebugGroup.h>
#include <Methane/Graphics/Null/ProgramBindings.h>
#include <Methane/Graphics/Null/Buffer.h>

#include <chrono>
#include <future>
#include <memory>
#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>
#include <thread>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

static const Platform::AppEnvironment test_app_env{ nullptr };
static const Rhi::RenderContextSettings render_context_settings = Test::GetRenderContextSettings();
static const Rhi::RenderPatternSettings render_pattern_settings = Test::GetRenderPatternSettings();

TEST_CASE("RHI Parallel Render Command List Functions", "[rhi][list][render]")
{
    const Rhi::RenderContext render_context   = Rhi::RenderContext(test_app_env, GetTestDevice(), g_parallel_executor, render_context_settings);
    const Rhi::CommandQueue  render_cmd_queue = render_context.CreateCommandQueue(Rhi::CommandListType::Render);
    const Rhi::RenderPattern render_pattern   = render_context.CreateRenderPattern(render_pattern_settings);
    const Rhi::Program       render_program   = [&render_context, &render_pattern]()
    {
        using enum Rhi::ShaderType;
        const Rhi::ProgramArgumentAccessor texture_accessor{ Pixel, "InTexture", Rhi::ProgramArgumentAccessType::Constant };
        const Rhi::ProgramArgumentAccessor sampler_accessor{ Pixel, "InSampler", Rhi::ProgramArgumentAccessType::Constant };
        const Rhi::ProgramArgumentAccessor buffer_accessor { Vertex, "OutBuffer", Rhi::ProgramArgumentAccessType::Mutable };
        Rhi::Program render_program = render_context.CreateProgram(
            Rhi::ProgramSettingsImpl
            {
                .shader_set = Rhi::ProgramSettingsImpl::ShaderSet
                {
                    { Vertex, { Data::ShaderProvider::Get(), { "Render", "MainVS" } } },
                    { Pixel,  { Data::ShaderProvider::Get(), { "Render", "MainPS" } } }
                },
                .input_buffer_layouts = Rhi::ProgramInputBufferLayouts
                {
                    Rhi::ProgramInputBufferLayout
                    {
                        .argument_semantics = Rhi::ProgramInputBufferLayout::ArgumentSemantics{ "POSITION" , "COLOR" },
                        .step_type = Rhi::ProgramInputBufferLayout::StepType::PerVertex,
                        .step_rate = 1U
                    },
                    Rhi::ProgramInputBufferLayout
                    {
                        .argument_semantics = Rhi::ProgramInputBufferLayout::ArgumentSemantics{ "NORMAL" , "TANGENT" },
                        .step_type = Rhi::ProgramInputBufferLayout::StepType::PerVertex,
                        .step_rate = 1U
                    }
                },
                .argument_accessors = Rhi::ProgramArgumentAccessors
                {
                    texture_accessor,
                    sampler_accessor,
                    buffer_accessor
                },
                .attachment_formats = render_pattern.GetAttachmentFormats()
            });
        dynamic_cast<Null::Program&>(render_program.GetInterface()).SetArgumentBindings({
            { texture_accessor, { Rhi::ResourceType::Texture, 1U } },
            { sampler_accessor, { Rhi::ResourceType::Sampler, 1U } },
            { buffer_accessor,  { Rhi::ResourceType::Buffer,  1U } },
        });
        return render_program;
    }();

    const Test::RenderPassResources render_pass_resources = Test::GetRenderPassResources(render_pattern);
    const Rhi::RenderPass render_pass = render_pattern.CreateRenderPass(render_pass_resources.settings);

    SECTION("Parallel Render Command List Construction")
    {
        Rhi::ParallelRenderCommandList cmd_list;
        REQUIRE_NOTHROW(cmd_list = render_cmd_queue.CreateParallelRenderCommandList(render_pass));
        REQUIRE(cmd_list.IsInitialized());
        CHECK(cmd_list.GetInterfacePtr());
        CHECK(cmd_list.GetCommandQueue().GetInterfacePtr().get() == render_cmd_queue.GetInterfacePtr().get());
        CHECK(cmd_list.GetRenderPass().GetInterfacePtr().get() == render_pass.GetInterfacePtr().get());
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Pending);
    }

    SECTION("Object Destroyed Callback")
    {
        auto cmd_list_ptr = std::make_unique<Rhi::ParallelRenderCommandList>(render_cmd_queue, render_pass);
        ObjectCallbackTester object_callback_tester(*cmd_list_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        cmd_list_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    const Rhi::ParallelRenderCommandList cmd_list = render_cmd_queue.CreateParallelRenderCommandList(render_pass);
    const auto& null_cmd_list = dynamic_cast<Null::ParallelRenderCommandList&>(cmd_list.GetInterface());

    SECTION("Object Name Setup")
    {
        CHECK(cmd_list.SetName("My Command List"));
        CHECK(cmd_list.GetName() == "My Command List");
    }

    SECTION("Object Name Change Callback")
    {
        CHECK(cmd_list.SetName("My Command List"));
        ObjectCallbackTester object_callback_tester(cmd_list);
        CHECK(cmd_list.SetName("Our Command List"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Command List");
        CHECK(object_callback_tester.GetOldObjectName() == "My Command List");
    }

    SECTION("Object Name Set Unchanged")
    {
        CHECK(cmd_list.SetName("My Command List"));
        ObjectCallbackTester object_callback_tester(cmd_list);
        CHECK_FALSE(cmd_list.SetName("My Command List"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Add to Objects Registry")
    {
        cmd_list.SetName("Parallel Render Command List");
        Rhi::ObjectRegistry registry = render_context.GetObjectRegistry();
        registry.AddGraphicsObject(cmd_list);
        const auto registered_cmd_list = registry.GetGraphicsObject<Rhi::ParallelRenderCommandList>("Parallel Render Command List");
        REQUIRE(registered_cmd_list.IsInitialized());
        CHECK(&registered_cmd_list.GetInterface() == &cmd_list.GetInterface());
    }

    SECTION("Reset Command List")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);
    }

    SECTION("Reset Command List Once")
    {
        REQUIRE_NOTHROW(cmd_list.ResetOnce());
        REQUIRE_NOTHROW(cmd_list.ResetOnce());
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);
    }

    SECTION("Push is Not Implemented")
    {
        CHECK_THROWS_AS(cmd_list.PushDebugGroup(Rhi::CommandListDebugGroup("Test")), NotImplementedException);
    }

    SECTION("Pop is Not Implemented")
    {
        CHECK_THROWS_AS(cmd_list.PushDebugGroup(Rhi::CommandListDebugGroup("Test")), NotImplementedException);
    }

    SECTION("Set Program Bindings is Not Implemented")
    {
        const Rhi::Texture texture = render_context.CreateTexture(
              Rhi::TextureSettings::ForImage(Dimensions(640, 480), {}, PixelFormat::RGBA8, false));
        const Rhi::Sampler sampler = render_context.CreateSampler({
                rhi::SamplerFilter  { rhi::SamplerFilter::MinMag::Linear },
                rhi::SamplerAddress { rhi::SamplerAddress::Mode::ClampToEdge }
        });
        const Rhi::Buffer buffer = render_context.CreateBuffer(
              Rhi::BufferSettings::ForConstantBuffer(42000, false, true));

        using enum Rhi::ShaderType;
        const Rhi::ProgramBindings render_program_bindings = render_program.CreateBindings({
            { { Pixel,  "InTexture" }, texture.GetResourceView() },
            { { Pixel,  "InSampler" }, sampler.GetResourceView() },
            { { Vertex, "OutBuffer" }, buffer.GetResourceView()  },
        });

        REQUIRE_THROWS_AS(cmd_list.SetProgramBindings(render_program_bindings), NotImplementedException);
    }

    SECTION("Set Resource Barriers is Not Implemented")
    {
        const Rhi::ResourceBarriers barriers(Rhi::IResourceBarriers::Set{});
        REQUIRE_THROWS_AS(cmd_list.SetResourceBarriers(barriers), NotImplementedException);
    }

    SECTION("Commit Command List")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK_NOTHROW(cmd_list.Commit());
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Committed);
    }

    SECTION("Execute Command List with Callback Tracker")
    {
        CommandListCallbackTester cmd_list_callback_tester(cmd_list);
        const Rhi::CommandListSet cmd_list_set({ cmd_list.GetInterface() });

        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);
        CHECK(cmd_list_callback_tester.GetTrackingState() == Rhi::CommandListState::Encoding);
        CHECK(cmd_list_callback_tester.IsStateChanged());
        CHECK_FALSE(cmd_list_callback_tester.IsExecutionCompleted());

        cmd_list_callback_tester.Reset();

        REQUIRE_NOTHROW(cmd_list.Commit());
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Committed);
        CHECK(cmd_list_callback_tester.GetTrackingState() == Rhi::CommandListState::Committed);
        CHECK(cmd_list_callback_tester.IsStateChanged());
        CHECK_FALSE(cmd_list_callback_tester.IsExecutionCompleted());

        cmd_list_callback_tester.Reset();

        Rhi::ICommandList* completed_command_list_ptr = nullptr;
        REQUIRE_NOTHROW(render_cmd_queue.Execute(cmd_list_set,
            [&completed_command_list_ptr](Rhi::ICommandList& command_list) {
                completed_command_list_ptr = &command_list;
            }));

        CHECK(cmd_list.GetState() == Rhi::CommandListState::Executing);
        CHECK(cmd_list_callback_tester.GetTrackingState() == Rhi::CommandListState::Executing);
        CHECK(cmd_list_callback_tester.IsStateChanged());
        CHECK_FALSE(cmd_list_callback_tester.IsExecutionCompleted());
        CHECK_FALSE(completed_command_list_ptr);

        cmd_list_callback_tester.Reset();
        dynamic_cast<Null::CommandListSet&>(cmd_list_set.GetInterface()).Complete();

        CHECK(cmd_list.GetState() == Rhi::CommandListState::Pending);
        CHECK(cmd_list_callback_tester.GetTrackingState() == Rhi::CommandListState::Pending);
        CHECK(cmd_list_callback_tester.IsExecutionCompleted());
        CHECK(completed_command_list_ptr == cmd_list.GetInterfacePtr().get());
    }

    SECTION("Wait Until Command List Completed")
    {
        const Rhi::CommandListSet cmd_list_set({ cmd_list.GetInterface() });
        REQUIRE_NOTHROW(cmd_list.Reset());
        REQUIRE_NOTHROW(cmd_list.Commit());
        REQUIRE_NOTHROW(render_cmd_queue.Execute(cmd_list_set));

        auto async_complete = std::async(std::launch::async, [&cmd_list_set]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            dynamic_cast<Null::CommandListSet&>(cmd_list_set.GetInterface()).Complete();
        });

        CHECK(cmd_list.GetState() == Rhi::CommandListState::Executing);
        REQUIRE_NOTHROW(cmd_list.WaitUntilCompleted());
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Pending);
    }

    SECTION("Get GPU Time Range")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK_NOTHROW(cmd_list.GetGpuTimeRange(true) == Data::TimeRange{});
        CHECK_NOTHROW(cmd_list.GetGpuTimeRange(false) == Data::TimeRange{});
    }

    SECTION("Disable Validation (to reduce overhead)")
    {
        CHECK(cmd_list.IsValidationEnabled());
        REQUIRE_NOTHROW(cmd_list.SetValidationEnabled(false));
        CHECK_FALSE(cmd_list.IsValidationEnabled());
    }

    SECTION("Set Parallel Render Command Lists Count")
    {
        CHECK(cmd_list.GetParallelCommandLists().size() == 0U);
        CHECK(cmd_list.SetName("Test"));
        REQUIRE_NOTHROW(cmd_list.SetParallelCommandListsCount(4U));

        const std::vector<Rhi::RenderCommandList> thread_cmd_lists = cmd_list.GetParallelCommandLists();
        REQUIRE(thread_cmd_lists.size() == 4U);

        uint32_t thread_index = 0U;
        for (const Rhi::RenderCommandList& thread_cmd_list : cmd_list.GetParallelCommandLists())
        {
            CHECK(thread_cmd_list.GetName() == fmt::format("Test - Thread {}", thread_index));
            thread_index++;
        }
    }

    REQUIRE_NOTHROW(cmd_list.SetParallelCommandListsCount(4U));
    const Rhi::RenderStateSettingsImpl render_state_settings = Test::GetRenderStateSettings(render_context, render_pattern, render_program);
    const Rhi::RenderState render_state = render_context.CreateRenderState(render_state_settings);

    SECTION("Reset Command List with Render State")
    {
        REQUIRE_NOTHROW(cmd_list.ResetWithState(render_state));
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);

        const std::vector<Rhi::RenderCommandList> thread_cmd_lists = cmd_list.GetParallelCommandLists();
        REQUIRE(thread_cmd_lists.size() == 4U);
        for (const Rhi::RenderCommandList& thread_cmd_list : cmd_list.GetParallelCommandLists())
        {
            CHECK(thread_cmd_list.GetState() == Rhi::CommandListState::Encoding);
            const auto& null_thread_cmd_list = dynamic_cast<Null::RenderCommandList&>(thread_cmd_list.GetInterface());
            CHECK(null_thread_cmd_list.GetDrawingState().render_state_ptr.get() == render_state.GetInterfacePtr().get());
        }
    }

    SECTION("Reset Command List with Render State and Debug Group")
    {
        const Rhi::CommandListDebugGroup debug_group("Test");
        REQUIRE_NOTHROW(cmd_list.ResetWithState(render_state, &debug_group));
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);

        uint32_t thread_index = 0U;
        const std::vector<Rhi::RenderCommandList> thread_cmd_lists = cmd_list.GetParallelCommandLists();
        for (const Rhi::RenderCommandList& thread_cmd_list : cmd_list.GetParallelCommandLists())
        {
            CHECK(thread_cmd_list.GetState() == Rhi::CommandListState::Encoding);
            const auto& null_thread_cmd_list = dynamic_cast<Null::RenderCommandList&>(thread_cmd_list.GetInterface());
            CHECK(null_thread_cmd_list.GetDrawingState().render_state_ptr.get() == render_state.GetInterfacePtr().get());

            const Opt<Rhi::CommandListDebugGroup> thread_debug_group_opt = debug_group.GetSubGroup(thread_index);
            CHECK(thread_debug_group_opt.has_value());
            CHECK(null_thread_cmd_list.GetTopOpenDebugGroup());
            CHECK(null_thread_cmd_list.GetTopOpenDebugGroup()->GetName() == thread_debug_group_opt->GetName());
            CHECK(thread_debug_group_opt->GetName() == fmt::format("Test - Thread {}", thread_index));
            thread_index++;
        }
    }

    const Rhi::ViewState view_state(Test::GetViewStateSettings());

    SECTION("Set View State")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        REQUIRE_NOTHROW(cmd_list.SetViewState(view_state));

        const std::vector<Rhi::RenderCommandList> thread_cmd_lists = cmd_list.GetParallelCommandLists();
        for (const Rhi::RenderCommandList& thread_cmd_list : cmd_list.GetParallelCommandLists())
        {
            CHECK(thread_cmd_list.GetState() == Rhi::CommandListState::Encoding);
            const auto& null_thread_cmd_list = dynamic_cast<Null::RenderCommandList&>(thread_cmd_list.GetInterface());
            CHECK(null_thread_cmd_list.GetDrawingState().view_state_ptr == &view_state.GetInterface());
            CHECK(null_thread_cmd_list.GetDrawingState().changes.HasAnyBit(Base::RenderDrawingState::Change::ViewState));
        }
    }

    const Rhi::ResourceBarriers barriers(Rhi::IResourceBarriers::Set{});

    SECTION("Set Beginning Resource Barriers")
    {
        CHECK_FALSE(null_cmd_list.GetBeginningResourceBarriers());
        REQUIRE_NOTHROW(cmd_list.SetBeginningResourceBarriers(barriers));
        CHECK(null_cmd_list.GetBeginningResourceBarriers() == &barriers.GetInterface());
    }

    SECTION("Set Ending Resource Barriers")
    {
        CHECK_FALSE(null_cmd_list.GetEndingResourceBarriers());
        REQUIRE_NOTHROW(cmd_list.SetEndingResourceBarriers(barriers));
        CHECK(null_cmd_list.GetEndingResourceBarriers() == &barriers.GetInterface());
    }
}
