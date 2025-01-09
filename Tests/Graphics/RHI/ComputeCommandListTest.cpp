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

FILE: Tests/Graphics/RHI/ComputeCommandListTest.cpp
Unit-tests of the RHI Compute Command List

******************************************************************************/

#include "RhiTestHelpers.hpp"

#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/ComputeCommandList.h>
#include <Methane/Graphics/RHI/ComputeState.h>
#include <Methane/Graphics/RHI/Program.h>
#include <Methane/Graphics/RHI/CommandListDebugGroup.h>
#include <Methane/Graphics/RHI/CommandListSet.h>
#include <Methane/Graphics/RHI/ResourceBarriers.h>
#include <Methane/Graphics/RHI/ProgramBindings.h>
#include <Methane/Graphics/RHI/Texture.h>
#include <Methane/Graphics/RHI/Buffer.h>
#include <Methane/Graphics/RHI/Sampler.h>
#include <Methane/Graphics/Null/CommandListSet.h>
#include <Methane/Graphics/Null/ComputeCommandList.h>
#include <Methane/Graphics/Null/Program.h>
#include <Methane/Graphics/Null/ComputeState.h>
#include <Methane/Graphics/Null/CommandListDebugGroup.h>
#include <Methane/Graphics/Null/ProgramBindings.h>

#include <chrono>
#include <future>
#include <memory>
#include <taskflow/taskflow.hpp>
#include <catch2/catch_test_macros.hpp>
#include <thread>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

TEST_CASE("RHI Compute Command List Functions", "[rhi][list][compute]")
{
    const Rhi::ComputeContext compute_context = Rhi::ComputeContext(GetTestDevice(), g_parallel_executor, {});
    const Rhi::CommandQueue compute_cmd_queue = compute_context.CreateCommandQueue(Rhi::CommandListType::Compute);

    const Rhi::Program compute_program = [&compute_context]()
    {
        using enum Rhi::ShaderType;
        const Rhi::ProgramArgumentAccessor texture_accessor{ Compute, "InTexture", Rhi::ProgramArgumentAccessType::Constant };
        const Rhi::ProgramArgumentAccessor sampler_accessor{ Compute, "InSampler", Rhi::ProgramArgumentAccessType::Constant };
        const Rhi::ProgramArgumentAccessor buffer_accessor { Compute, "OutBuffer", Rhi::ProgramArgumentAccessType::Mutable };
        Rhi::Program compute_program = compute_context.CreateProgram(
            Rhi::ProgramSettingsImpl
            {
                Rhi::ProgramSettingsImpl::ShaderSet
                {
                    { Compute, { Data::ShaderProvider::Get(), { "Compute", "Main" } } }
                },
                Rhi::ProgramInputBufferLayouts{ },
                Rhi::ProgramArgumentAccessors
                {
                    texture_accessor,
                    sampler_accessor,
                    buffer_accessor
                }
            });
        dynamic_cast<Null::Program&>(compute_program.GetInterface()).SetArgumentBindings({
            { texture_accessor, { Rhi::ResourceType::Texture, 1U } },
            { sampler_accessor, { Rhi::ResourceType::Sampler, 1U } },
            { buffer_accessor,  { Rhi::ResourceType::Buffer,  1U } },
        });
        return compute_program;
    }();

    const Rhi::ComputeState compute_state = compute_context.CreateComputeState({
        compute_program,
        Rhi::ThreadGroupSize(16, 16, 1)
    });

    SECTION("Transfer Command List Construction")
    {
        Rhi::ComputeCommandList cmd_list;
        REQUIRE_NOTHROW(cmd_list = compute_cmd_queue.CreateComputeCommandList());
        REQUIRE(cmd_list.IsInitialized());
        CHECK(cmd_list.GetInterfacePtr());
        CHECK(cmd_list.GetCommandQueue().GetInterfacePtr().get() == compute_cmd_queue.GetInterfacePtr().get());
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Pending);
    }

    SECTION("Object Destroyed Callback")
    {
        auto cmd_list_ptr = std::make_unique<Rhi::ComputeCommandList>(compute_cmd_queue);
        ObjectCallbackTester object_callback_tester(*cmd_list_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        cmd_list_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    const Rhi::ComputeCommandList cmd_list = compute_cmd_queue.CreateComputeCommandList();

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
        CHECK(cmd_list.SetName("My Fence"));
        ObjectCallbackTester object_callback_tester(cmd_list);
        CHECK_FALSE(cmd_list.SetName("My Fence"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
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
        CHECK(dynamic_cast<Null::ComputeCommandList&>(cmd_list.GetInterface()).GetTopOpenDebugGroup()->GetName() == "Test");
    }

    SECTION("Reset Command List Once with Debug Group")
    {
        const Rhi::CommandListDebugGroup debug_group("Test");
        REQUIRE_NOTHROW(cmd_list.ResetOnce(&debug_group));
        REQUIRE_NOTHROW(cmd_list.ResetOnce(&debug_group));
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);
        CHECK(dynamic_cast<Null::ComputeCommandList&>(cmd_list.GetInterface()).GetTopOpenDebugGroup()->GetName() == "Test");
    }

    SECTION("Push and Pop Debug Group")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK_NOTHROW(cmd_list.PushDebugGroup(Rhi::CommandListDebugGroup("Test")));
        CHECK_NOTHROW(cmd_list.PopDebugGroup());
    }

    SECTION("Can not Pop Missing Debug Group")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        CHECK_THROWS(cmd_list.PopDebugGroup());
    }

    SECTION("Set Program Bindings")
    {
        const Rhi::Texture texture = [&compute_context]()
        {
            Rhi::Texture texture = compute_context.CreateTexture(Rhi::TextureSettings::ForImage(Dimensions(640, 480), {}, PixelFormat::RGBA8, false));
            texture.SetName("T");
            return texture;
        }();

        const Rhi::Sampler sampler = [&compute_context]()
        {
            const Rhi::Sampler sampler = compute_context.CreateSampler({
                rhi::SamplerFilter  { rhi::SamplerFilter::MinMag::Linear },
                rhi::SamplerAddress { rhi::SamplerAddress::Mode::ClampToEdge }
            });
            sampler.SetName("S");
            return sampler;
        }();

        const Rhi::Buffer buffer = [&compute_context]()
        {
            const Rhi::Buffer buffer = compute_context.CreateBuffer(Rhi::BufferSettings::ForConstantBuffer(42000, false, true));
            buffer.SetName("B");
            return buffer;
        }();

        using enum Rhi::ShaderType;
        const Rhi::ProgramBindings compute_program_bindings = compute_program.CreateBindings({
            { { Compute, "InTexture" }, texture.GetResourceView() },
            { { Compute, "InSampler" }, sampler.GetResourceView() },
            { { Compute, "OutBuffer" }, buffer.GetResourceView()  },
        });

        REQUIRE_NOTHROW(cmd_list.ResetWithState(compute_state));
        REQUIRE_NOTHROW(cmd_list.SetProgramBindings(compute_program_bindings));
        REQUIRE_NOTHROW(cmd_list.Commit());
        CHECK(dynamic_cast<Null::ComputeCommandList&>(cmd_list.GetInterface()).GetProgramBindingsPtr() == compute_program_bindings.GetInterfacePtr().get());
    }

    SECTION("Set Resource Barriers")
    {
        const Rhi::ResourceBarriers barriers(Rhi::IResourceBarriers::Set{});
        REQUIRE_NOTHROW(cmd_list.Reset());
        REQUIRE_NOTHROW(cmd_list.SetResourceBarriers(barriers.GetInterface()));
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
        REQUIRE_NOTHROW(compute_cmd_queue.Execute(cmd_list_set,
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
        REQUIRE_NOTHROW(compute_cmd_queue.Execute(cmd_list_set));

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

    SECTION("Reset Command List with Compute State")
    {
        REQUIRE_NOTHROW(cmd_list.ResetWithState(compute_state));
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);
        CHECK(&dynamic_cast<Null::ComputeCommandList&>(cmd_list.GetInterface()).GetComputeState() == compute_state.GetInterfacePtr().get());
    }

    SECTION("Reset Command List Once with Compute State")
    {
        REQUIRE_NOTHROW(cmd_list.ResetWithStateOnce(compute_state));
        REQUIRE_NOTHROW(cmd_list.ResetWithStateOnce(compute_state));
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);
        CHECK(&dynamic_cast<Null::ComputeCommandList&>(cmd_list.GetInterface()).GetComputeState() == compute_state.GetInterfacePtr().get());
    }

    SECTION("Reset Command List with Compute State and Debug Group")
    {
        const Rhi::CommandListDebugGroup debug_group("Test");
        REQUIRE_NOTHROW(cmd_list.ResetWithState(compute_state, &debug_group));
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);

        auto& null_cmd_list = dynamic_cast<Null::ComputeCommandList&>(cmd_list.GetInterface());
        CHECK(null_cmd_list.GetTopOpenDebugGroup()->GetName() == "Test");
        CHECK(&null_cmd_list.GetComputeState() == compute_state.GetInterfacePtr().get());
    }

    SECTION("Reset Command List Once with Compute State and Debug Group")
    {
        const Rhi::CommandListDebugGroup debug_group("Test");
        REQUIRE_NOTHROW(cmd_list.ResetWithStateOnce(compute_state, &debug_group));
        REQUIRE_NOTHROW(cmd_list.ResetWithStateOnce(compute_state, &debug_group));
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);

        auto& null_cmd_list = dynamic_cast<Null::ComputeCommandList&>(cmd_list.GetInterface());
        CHECK(null_cmd_list.GetTopOpenDebugGroup()->GetName() == "Test");
        CHECK(&null_cmd_list.GetComputeState() == compute_state.GetInterfacePtr().get());
    }

    SECTION("Set Command List Compute State")
    {
        REQUIRE_NOTHROW(cmd_list.Reset());
        REQUIRE_NOTHROW(cmd_list.SetComputeState(compute_state));
        CHECK(cmd_list.GetState() == Rhi::CommandListState::Encoding);

        auto& null_cmd_list = dynamic_cast<Null::ComputeCommandList&>(cmd_list.GetInterface());
        CHECK(&null_cmd_list.GetComputeState() == compute_state.GetInterfacePtr().get());
    }

    SECTION("Dispatch thread groups in Compute Command List")
    {
        const Rhi::CommandListSet cmd_list_set({ cmd_list.GetInterface() });
        REQUIRE_NOTHROW(cmd_list.ResetWithState(compute_state));
        REQUIRE_NOTHROW(cmd_list.Dispatch(Rhi::ThreadGroupsCount(4U, 4U, 1U)));
        REQUIRE_NOTHROW(cmd_list.Commit());
        REQUIRE_NOTHROW(compute_cmd_queue.Execute(cmd_list_set));
        dynamic_cast<Null::CommandListSet&>(cmd_list_set.GetInterface()).Complete();
    }
}
