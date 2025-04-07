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

FILE: Tests/Graphics/RHI/RenderCommandListTest.cpp
Unit-tests of the RHI Render Command List

******************************************************************************/

#include "RhiTestHelpers.hpp"
#include "RhiSettings.hpp"

#include <Methane/Data/EnumMaskUtil.hpp>
#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/RenderCommandList.h>
#include <Methane/Graphics/RHI/RenderState.h>
#include <Methane/Graphics/RHI/ViewState.h>
#include <Methane/Graphics/RHI/Program.h>
#include <Methane/Graphics/RHI/CommandListDebugGroup.h>
#include <Methane/Graphics/RHI/CommandListSet.h>
#include <Methane/Graphics/RHI/ResourceBarriers.h>
#include <Methane/Graphics/RHI/ProgramBindings.h>
#include <Methane/Graphics/RHI/Texture.h>
#include <Methane/Graphics/RHI/Buffer.h>
#include <Methane/Graphics/RHI/BufferSet.h>
#include <Methane/Graphics/RHI/Sampler.h>
#include <Methane/Graphics/RHI/ObjectRegistry.h>
#include <Methane/Graphics/Base/ViewState.h>
#include <Methane/Graphics/Base/BufferSet.h>
#include <Methane/Graphics/Base/Buffer.h>
#include <Methane/Graphics/Null/CommandListSet.h>
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

template<typename E, typename M>
struct Catch::StringMaker<Data::EnumMask<E, M>>
{
    static std::string convert(const Data::EnumMask<E, M>& mask)
    {
        return Data::GetEnumMaskName(mask);
    }
};

static tf::Executor g_parallel_executor;

static const Platform::AppEnvironment test_app_env{ nullptr };
static const Rhi::RenderContextSettings render_context_settings = Test::GetRenderContextSettings();
static const Rhi::RenderPatternSettings render_pattern_settings = Test::GetRenderPatternSettings();

TEST_CASE("RHI Render Command List Functions", "[rhi][list][render]")
{
    const Rhi::RenderContext render_context   = Rhi::RenderContext(test_app_env, GetTestDevice(), g_parallel_executor, render_context_settings);
    const Rhi::CommandQueue  render_cmd_queue = render_context.CreateCommandQueue(Rhi::CommandListType::Render);
    const Rhi::RenderPattern render_pattern = render_context.CreateRenderPattern(render_pattern_settings);
    const Rhi::Program render_program = [&render_context, &render_pattern]()
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

    SECTION("Render Command List Construction")
    {
        Rhi::RenderCommandList cmd_list;
        REQUIRE_NOTHROW(cmd_list = render_cmd_queue.CreateRenderCommandList(render_pass));
        REQUIRE(cmd_list.IsInitialized());
        CHECK(cmd_list.GetInterfacePtr());
        CHECK(cmd_list.GetCommandQueue().GetInterfacePtr().get() == render_cmd_queue.GetInterfacePtr().get());
        CHECK(cmd_list.GetRenderPass().GetInterfacePtr().get() == render_pass.GetInterfacePtr().get());
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Pending);
    }

    SECTION("Object Destroyed Callback")
    {
        auto cmd_list_ptr = std::make_unique<Rhi::RenderCommandList>(render_cmd_queue, render_pass);
        ObjectCallbackTester object_callback_tester(*cmd_list_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        cmd_list_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    const Rhi::RenderCommandList cmd_list = render_cmd_queue.CreateRenderCommandList(render_pass);
    const auto& null_cmd_list = dynamic_cast<Null::RenderCommandList&>(cmd_list.GetInterface());

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
        cmd_list.SetName("Render Command List");
        Rhi::ObjectRegistry registry = render_context.GetObjectRegistry();
        registry.AddGraphicsObject(cmd_list);
        const auto registered_cmd_list = registry.GetGraphicsObject<Rhi::RenderCommandList>("Render Command List");
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

    SECTION("Reset Command List with Debug Group")
    {
        const Rhi::CommandListDebugGroup debug_group("Test");
        REQUIRE_NOTHROW(cmd_list.Reset(&debug_group));
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);
        REQUIRE(null_cmd_list.GetTopOpenDebugGroup() != nullptr);
        CHECK(null_cmd_list.GetTopOpenDebugGroup()->GetName() == "Test");
    }

    SECTION("Reset Command List Once with Debug Group")
    {
        const Rhi::CommandListDebugGroup debug_group("Test");
        REQUIRE_NOTHROW(cmd_list.ResetOnce(&debug_group));
        REQUIRE_NOTHROW(cmd_list.ResetOnce(&debug_group));
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);
        REQUIRE(null_cmd_list.GetTopOpenDebugGroup() != nullptr);
        CHECK(null_cmd_list.GetTopOpenDebugGroup()->GetName() == "Test");
    }

    SECTION("Push and Pop Debug Group")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK_NOTHROW(cmd_list.PushDebugGroup(Rhi::CommandListDebugGroup("Test")));
        CHECK_NOTHROW(cmd_list.PopDebugGroup());
        REQUIRE(null_cmd_list.GetTopOpenDebugGroup() == nullptr);
    }

    SECTION("Can not Pop Missing Debug Group")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK_THROWS(cmd_list.PopDebugGroup());
    }

    SECTION("Set Program Bindings")
    {
        const Rhi::Texture texture = [&render_context]()
        {
            Rhi::Texture texture = render_context.CreateTexture(Rhi::TextureSettings::ForImage(Dimensions(640, 480), {}, PixelFormat::RGBA8, false));
            texture.SetName("T");
            return texture;
        }();

        const Rhi::Sampler sampler = [&render_context]()
        {
            const Rhi::Sampler sampler = render_context.CreateSampler({
                rhi::SamplerFilter  { rhi::SamplerFilter::MinMag::Linear },
                rhi::SamplerAddress { rhi::SamplerAddress::Mode::ClampToEdge }
            });
            sampler.SetName("S");
            return sampler;
        }();

        const Rhi::Buffer buffer = [&render_context]()
        {
            const Rhi::Buffer buffer = render_context.CreateBuffer(Rhi::BufferSettings::ForConstantBuffer(42000, false, true));
            buffer.SetName("B");
            return buffer;
        }();

        using enum Rhi::ShaderType;
        const Rhi::ProgramBindings render_program_bindings = render_program.CreateBindings({
            { { Pixel,  "InTexture" }, texture.GetResourceView() },
            { { Pixel,  "InSampler" }, sampler.GetResourceView() },
            { { Vertex, "OutBuffer" }, buffer.GetResourceView()  },
        });

        REQUIRE_NOTHROW(cmd_list.Reset());
        REQUIRE_NOTHROW(cmd_list.SetProgramBindings(render_program_bindings));
        CHECK(dynamic_cast<Null::RenderCommandList&>(cmd_list.GetInterface()).GetProgramBindingsPtr() == render_program_bindings.GetInterfacePtr().get());
    }

    SECTION("Set Resource Barriers")
    {
        const Rhi::ResourceBarriers barriers(Rhi::IResourceBarriers::Set{});
        REQUIRE_NOTHROW(cmd_list.Reset());
        REQUIRE_NOTHROW(cmd_list.SetResourceBarriers(barriers));
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

    const Rhi::RenderStateSettingsImpl render_state_settings = Test::GetRenderStateSettings(render_context, render_pattern, render_program);
    const Rhi::RenderState render_state = render_context.CreateRenderState(render_state_settings);
    const auto& null_render_state = dynamic_cast<Null::RenderState&>(render_state.GetInterface());

    SECTION("Reset Command List with Render State")
    {
        REQUIRE_NOTHROW(cmd_list.ResetWithState(render_state));
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);
        CHECK(null_cmd_list.GetDrawingState().render_state_ptr.get() == render_state.GetInterfacePtr().get());
    }

    SECTION("Reset Command List Once with Render State")
    {
        REQUIRE_NOTHROW(cmd_list.ResetWithStateOnce(render_state));
        REQUIRE_NOTHROW(cmd_list.ResetWithStateOnce(render_state));
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);
        CHECK(null_cmd_list.GetDrawingState().render_state_ptr.get() == render_state.GetInterfacePtr().get());
        CHECK(null_render_state.GetAppliedStateGroups().GetValue() == ~0U);
    }

    SECTION("Reset Command List with Render State and Debug Group")
    {
        const Rhi::CommandListDebugGroup debug_group("Test");
        REQUIRE_NOTHROW(cmd_list.ResetWithState(render_state, &debug_group));
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);
        CHECK(null_cmd_list.GetTopOpenDebugGroup()->GetName() == "Test");
        CHECK(null_cmd_list.GetDrawingState().render_state_ptr.get() == render_state.GetInterfacePtr().get());
        CHECK(null_render_state.GetAppliedStateGroups().GetValue() == ~0U);
    }

    SECTION("Reset Command List Once with Render State and Debug Group")
    {
        const Rhi::CommandListDebugGroup debug_group1("Test1");
        REQUIRE_NOTHROW(cmd_list.ResetWithStateOnce(render_state, &debug_group1));

        const Rhi::CommandListDebugGroup debug_group2("Test2");
        REQUIRE_NOTHROW(cmd_list.ResetWithStateOnce(render_state, &debug_group2));
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);
        CHECK(null_cmd_list.GetTopOpenDebugGroup()->GetName() == "Test1");
        CHECK(null_cmd_list.GetDrawingState().render_state_ptr.get() == render_state.GetInterfacePtr().get());
    }

    SECTION("Set Command List Render State after Stateless Reset")
    {
        const Rhi::RenderStateGroupMask state_groups{
            Rhi::RenderStateGroup::Rasterizer,
            Rhi::RenderStateGroup::Blending,
            Rhi::RenderStateGroup::DepthStencil
        };
        REQUIRE_NOTHROW(cmd_list.Reset());
        REQUIRE_NOTHROW(cmd_list.SetRenderState(render_state, state_groups));
        CHECK(null_cmd_list.GetDrawingState().render_state_ptr.get() == render_state.GetInterfacePtr().get());
        CHECK(null_cmd_list.GetDrawingState().render_state_groups == state_groups);
        CHECK(null_render_state.GetAppliedStateGroups() == state_groups);
    }

    SECTION("Change Command List Render State after Stateful Reset, Only Changed State Groups are Applied")
    {
        const Rhi::RenderStateSettingsImpl other_render_state_settings = Test::GetRenderStateSettings(render_context, render_pattern, render_program,
            Rhi::RasterizerSettings
            {
                .is_front_counter_clockwise = false,
                .cull_mode                  = Rhi::RasterizerCullMode::None,
                .fill_mode                  = Rhi::RasterizerFillMode::Wireframe
            },
            Rhi::DepthSettings
            {
                .enabled       = false,
                .write_enabled = false
            },
            Rhi::StencilSettings
            {
                .enabled = false
            });
        const Rhi::RenderState other_render_state = render_context.CreateRenderState(other_render_state_settings);
        const auto& other_null_render_state = dynamic_cast<Null::RenderState&>(other_render_state.GetInterface());

        REQUIRE_NOTHROW(cmd_list.ResetWithState(render_state));
        CHECK(null_cmd_list.GetDrawingState().render_state_ptr.get() == render_state.GetInterfacePtr().get());
        CHECK(null_cmd_list.GetDrawingState().render_state_groups.GetValue() == ~0U);
        CHECK(null_render_state.GetAppliedStateGroups().GetValue() == ~0U);

        REQUIRE_NOTHROW(cmd_list.SetRenderState(other_render_state));
        CHECK(null_cmd_list.GetDrawingState().render_state_ptr.get() == other_render_state.GetInterfacePtr().get());
        CHECK(null_cmd_list.GetDrawingState().render_state_groups.GetValue() == ~0U);
        CHECK(other_null_render_state.GetAppliedStateGroups() == Rhi::RenderStateGroupMask{
            Rhi::RenderStateGroup::Rasterizer,
            Rhi::RenderStateGroup::DepthStencil
        });
    }

    const Rhi::ViewState view_state(Test::GetViewStateSettings());

    SECTION("Set View State")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        REQUIRE_NOTHROW(cmd_list.SetViewState(view_state));
        CHECK(null_cmd_list.GetDrawingState().view_state_ptr == &view_state.GetInterface());
        CHECK(null_cmd_list.GetDrawingState().changes.HasAnyBit(Base::RenderDrawingState::Change::ViewState));
    }

    SECTION("Set Other View State with Same Settings is Ignored")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        REQUIRE_NOTHROW(cmd_list.SetViewState(view_state));
        CHECK(null_cmd_list.GetDrawingState().view_state_ptr == &view_state.GetInterface());
        CHECK(null_cmd_list.GetDrawingState().changes.HasAnyBit(Base::RenderDrawingState::Change::ViewState));

        const Rhi::ViewState other_view_state(Test::GetViewStateSettings());
        REQUIRE_NOTHROW(cmd_list.SetViewState(other_view_state));
        CHECK(null_cmd_list.GetDrawingState().view_state_ptr == &view_state.GetInterface());
        CHECK(null_cmd_list.GetDrawingState().changes.HasAnyBit(Base::RenderDrawingState::Change::ViewState));
    }

    Rhi::Buffer vertex_buffer_one = [&render_context]()
    {
        Rhi::Buffer vertex_buffer = render_context.CreateBuffer(Rhi::BufferSettings::ForVertexBuffer(144U, 12U, true));
        vertex_buffer.SetName("Vertex Buffer 1");
        dynamic_cast<Null::Buffer&>(vertex_buffer.GetInterface()).SetInitializedDataSize(144U * 12U);
        return vertex_buffer;
    }();
    Rhi::Buffer vertex_buffer_two = [&render_context]()
    {
        Rhi::Buffer vertex_buffer = render_context.CreateBuffer(Rhi::BufferSettings::ForVertexBuffer(345U, 12U, true));
        vertex_buffer.SetName("Vertex Buffer 2");
        dynamic_cast<Null::Buffer&>(vertex_buffer.GetInterface()).SetInitializedDataSize(234U * 12U);
        return vertex_buffer;
    }();
    const Rhi::BufferSet vertex_buffer_set = Rhi::BufferSet(Rhi::BufferType::Vertex, { vertex_buffer_one, vertex_buffer_two });

    SECTION("Set Vertex Buffers")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK(cmd_list.SetVertexBuffers(vertex_buffer_set));
        CHECK(null_cmd_list.GetDrawingState().vertex_buffer_set_ptr.get() == vertex_buffer_set.GetInterfacePtr().get());
        CHECK(IsResourceRetainedByCommandList<Null::RenderCommandList>(vertex_buffer_set, cmd_list));
    }

    SECTION("Change Vertex Buffers After Set")
    {
        const Rhi::BufferSet other_vertex_buffer_set = Rhi::BufferSet(Rhi::BufferType::Vertex, { vertex_buffer_one });
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK(cmd_list.SetVertexBuffers(vertex_buffer_set));
        CHECK(cmd_list.SetVertexBuffers(other_vertex_buffer_set));
        CHECK(null_cmd_list.GetDrawingState().vertex_buffer_set_ptr.get() == other_vertex_buffer_set.GetInterfacePtr().get());
        CHECK(IsResourceRetainedByCommandList<Null::RenderCommandList>(vertex_buffer_set, cmd_list));
        CHECK(IsResourceRetainedByCommandList<Null::RenderCommandList>(other_vertex_buffer_set, cmd_list));
    }

    SECTION("Set Same Vertex Buffers Twice is Ignored")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK(cmd_list.SetVertexBuffers(vertex_buffer_set));
        CHECK_FALSE(cmd_list.SetVertexBuffers(vertex_buffer_set));
    }

    SECTION("Can Not Set Vertex Buffers with Constant Buffers")
    {
        Rhi::Buffer          constant_buffer_one  = render_context.CreateBuffer(Rhi::BufferSettings::ForConstantBuffer(421, true, true));
        const Rhi::BufferSet constant_buffer_set = Rhi::BufferSet(Rhi::BufferType::Constant, { constant_buffer_one });
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK_THROWS_AS(cmd_list.SetVertexBuffers(constant_buffer_set), ArgumentException);
    }

    Rhi::Buffer index_buffer_one = [&render_context]()
    {
        Rhi::Buffer index_buffer = render_context.CreateBuffer(Rhi::BufferSettings::ForIndexBuffer(543, PixelFormat::R16Uint));
        index_buffer.SetName("Index Buffer 1");
        dynamic_cast<Null::Buffer&>(index_buffer.GetInterface()).SetInitializedDataSize(234U * 2U);
        return index_buffer;
    }();

    SECTION("Set Index Buffer")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK(cmd_list.SetIndexBuffer(index_buffer_one));
        CHECK(dynamic_cast<const Rhi::IBuffer*>(null_cmd_list.GetDrawingState().index_buffer_ptr.get()) == index_buffer_one.GetInterfacePtr().get());
        CHECK(IsResourceRetainedByCommandList<Null::RenderCommandList>(index_buffer_one, cmd_list));
    }

    SECTION("Change Index Buffer After Set")
    {
        Rhi::Buffer index_buffer_two = render_context.CreateBuffer(Rhi::BufferSettings::ForIndexBuffer(543, PixelFormat::R16Uint));
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK(cmd_list.SetIndexBuffer(index_buffer_one));
        CHECK(cmd_list.SetIndexBuffer(index_buffer_two));
        CHECK(dynamic_cast<const Rhi::IBuffer*>(null_cmd_list.GetDrawingState().index_buffer_ptr.get()) == index_buffer_two.GetInterfacePtr().get());
        CHECK(IsResourceRetainedByCommandList<Null::RenderCommandList>(index_buffer_one, cmd_list));
        CHECK(IsResourceRetainedByCommandList<Null::RenderCommandList>(index_buffer_two, cmd_list));
    }

    SECTION("Set Same Index Buffer Twice is Ignored")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK(cmd_list.SetIndexBuffer(index_buffer_one));
        CHECK_FALSE(cmd_list.SetIndexBuffer(index_buffer_one));
    }

    SECTION("Can Not Set Index Buffer with Vertex Buffer")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK_THROWS_AS(cmd_list.SetIndexBuffer(vertex_buffer_one), ArgumentException);
    }

    SECTION("Can Draw Triangles from Vertex Buffers")
    {
        REQUIRE_NOTHROW(cmd_list.ResetWithState(render_state));
        REQUIRE_NOTHROW(cmd_list.SetViewState(view_state));
        REQUIRE(cmd_list.SetVertexBuffers(vertex_buffer_set));
        CHECK_FALSE(null_cmd_list.GetDrawingState().primitive_type_opt.has_value());
        REQUIRE_NOTHROW(cmd_list.Draw(Rhi::RenderPrimitive::Triangle, 100U, 10U, 12U, 3U));
        CHECK(null_cmd_list.GetDrawingState().primitive_type_opt == Rhi::RenderPrimitive::Triangle);
    }

    SECTION("Can Not Draw Triangles from Uninitialized Vertex Buffers")
    {
        dynamic_cast<Null::Buffer&>(vertex_buffer_one.GetInterface()).SetInitializedDataSize(0U);
        dynamic_cast<Null::Buffer&>(vertex_buffer_two.GetInterface()).SetInitializedDataSize(0U);

        REQUIRE_NOTHROW(cmd_list.ResetWithState(render_state));
        REQUIRE_NOTHROW(cmd_list.SetViewState(view_state));
        REQUIRE(cmd_list.SetVertexBuffers(vertex_buffer_set));
        CHECK_THROWS_AS(cmd_list.Draw(Rhi::RenderPrimitive::Triangle, 100U, 10U, 12U, 3U), ArgumentException);
    }

    SECTION("Can Not Draw Triangles Without Render State")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        REQUIRE_NOTHROW(cmd_list.SetViewState(view_state));
        REQUIRE(cmd_list.SetVertexBuffers(vertex_buffer_set));
        CHECK_THROWS_AS(cmd_list.Draw(Rhi::RenderPrimitive::Triangle, 100U, 10U, 12U, 3U), ArgumentException);
    }

    SECTION("Can Not Draw Triangles Without View State")
    {
        REQUIRE_NOTHROW(cmd_list.ResetWithState(render_state));
        REQUIRE(cmd_list.SetVertexBuffers(vertex_buffer_set));
        CHECK_THROWS_AS(cmd_list.Draw(Rhi::RenderPrimitive::Triangle, 100U, 10U, 12U, 3U), ArgumentException);
    }

    SECTION("Can Not Draw Triangles Without Vertex Buffers")
    {
        REQUIRE_NOTHROW(cmd_list.ResetWithState(render_state));
        REQUIRE_NOTHROW(cmd_list.SetViewState(view_state));
        CHECK_THROWS_AS(cmd_list.Draw(Rhi::RenderPrimitive::Triangle, 100U, 10U, 12U, 3U), ArgumentException);
    }

    SECTION("Can Not Draw Triangles with More Vertices than Available in Vertex Buffers")
    {
        REQUIRE_NOTHROW(cmd_list.ResetWithState(render_state));
        REQUIRE_NOTHROW(cmd_list.SetViewState(view_state));
        REQUIRE(cmd_list.SetVertexBuffers(vertex_buffer_set));
        CHECK_THROWS_AS(cmd_list.Draw(Rhi::RenderPrimitive::Triangle, 158U, 0U, 12U, 3U), ArgumentException);
        CHECK_THROWS_AS(cmd_list.Draw(Rhi::RenderPrimitive::Triangle, 138U, 10U, 12U, 3U), ArgumentException);
    }

    const Data::Size indices_count = index_buffer_one.GetFormattedItemsCount();

    SECTION("Can Draw Indexed Triangles from Vertex Buffers")
    {
        REQUIRE_NOTHROW(cmd_list.ResetWithState(render_state));
        REQUIRE_NOTHROW(cmd_list.SetViewState(view_state));
        REQUIRE(cmd_list.SetVertexBuffers(vertex_buffer_set));
        REQUIRE(cmd_list.SetIndexBuffer(index_buffer_one));
        CHECK_FALSE(null_cmd_list.GetDrawingState().primitive_type_opt.has_value());
        REQUIRE_NOTHROW(cmd_list.DrawIndexed(Rhi::RenderPrimitive::Triangle, indices_count - 10U, 10U, 42U, 12U, 3U));
        CHECK(null_cmd_list.GetDrawingState().primitive_type_opt == Rhi::RenderPrimitive::Triangle);
    }

    SECTION("Can Not Draw Indexed Triangles from Uninitialized Vertex Buffers")
    {
        dynamic_cast<Null::Buffer&>(vertex_buffer_one.GetInterface()).SetInitializedDataSize(0U);
        dynamic_cast<Null::Buffer&>(vertex_buffer_two.GetInterface()).SetInitializedDataSize(0U);

        REQUIRE_NOTHROW(cmd_list.ResetWithState(render_state));
        REQUIRE_NOTHROW(cmd_list.SetViewState(view_state));
        REQUIRE(cmd_list.SetVertexBuffers(vertex_buffer_set));
        REQUIRE(cmd_list.SetIndexBuffer(index_buffer_one));
        CHECK_THROWS_AS(cmd_list.DrawIndexed(Rhi::RenderPrimitive::Triangle, indices_count - 10U, 10U, 42U, 12U, 3U), ArgumentException);
    }

    SECTION("Can Not Draw Indexed Triangles Without Render State")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        REQUIRE_NOTHROW(cmd_list.SetViewState(view_state));
        REQUIRE(cmd_list.SetVertexBuffers(vertex_buffer_set));
        REQUIRE(cmd_list.SetIndexBuffer(index_buffer_one));
        CHECK_THROWS_AS(cmd_list.DrawIndexed(Rhi::RenderPrimitive::Triangle, indices_count - 10U, 10U, 42U, 12U, 3U), ArgumentException);
    }

    SECTION("Can Not Draw Indexed Triangles Without View State")
    {
        REQUIRE_NOTHROW(cmd_list.ResetWithState(render_state));
        REQUIRE(cmd_list.SetVertexBuffers(vertex_buffer_set));
        REQUIRE(cmd_list.SetIndexBuffer(index_buffer_one));
        CHECK_THROWS_AS(cmd_list.DrawIndexed(Rhi::RenderPrimitive::Triangle, indices_count - 10U, 10U, 42U, 12U, 3U), ArgumentException);
    }

    SECTION("Can Not Draw Indexed Triangles Without Vertex Buffers")
    {
        REQUIRE_NOTHROW(cmd_list.ResetWithState(render_state));
        REQUIRE_NOTHROW(cmd_list.SetViewState(view_state));
        REQUIRE(cmd_list.SetIndexBuffer(index_buffer_one));
        CHECK_THROWS_AS(cmd_list.DrawIndexed(Rhi::RenderPrimitive::Triangle, indices_count - 10U, 10U, 42U, 12U, 3U), ArgumentException);
    }

    SECTION("Can Not Draw Indexed Triangles Without Index Buffer")
    {
        REQUIRE_NOTHROW(cmd_list.ResetWithState(render_state));
        REQUIRE_NOTHROW(cmd_list.SetViewState(view_state));
        REQUIRE(cmd_list.SetVertexBuffers(vertex_buffer_set));
        CHECK_THROWS_AS(cmd_list.DrawIndexed(Rhi::RenderPrimitive::Triangle, indices_count - 10U, 10U, 42U, 12U, 3U), ArgumentException);
    }

    SECTION("Can Not Draw Indexed Triangles with More Vertices Than Available in Vertex Buffers")
    {
        REQUIRE_NOTHROW(cmd_list.ResetWithState(render_state));
        REQUIRE_NOTHROW(cmd_list.SetViewState(view_state));
        REQUIRE(cmd_list.SetVertexBuffers(vertex_buffer_set));
        REQUIRE(cmd_list.SetIndexBuffer(index_buffer_one));
        CHECK_THROWS_AS(cmd_list.DrawIndexed(Rhi::RenderPrimitive::Triangle, indices_count, 0U, 200U, 12U, 3U), ArgumentException);
        CHECK_THROWS_AS(cmd_list.DrawIndexed(Rhi::RenderPrimitive::Triangle, indices_count, 10U, 42U, 12U, 3U), ArgumentException);
        CHECK_THROWS_AS(cmd_list.DrawIndexed(Rhi::RenderPrimitive::Triangle, indices_count - 10U, 30U, 42U, 12U, 3U), ArgumentException);
    }
}
