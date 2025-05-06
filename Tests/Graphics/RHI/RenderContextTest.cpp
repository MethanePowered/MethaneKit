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

FILE: Tests/Graphics/RHI/RenderContextTest.cpp
Unit-tests of the RHI RenderContext

******************************************************************************/

#include "RhiTestHelpers.hpp"
#include "RhiSettings.hpp"

#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/CommandKit.h>
#include <Methane/Graphics/RHI/TransferCommandList.h>
#include <Methane/Graphics/RHI/System.h>
#include <Methane/Graphics/RHI/Device.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/RenderState.h>
#include <Methane/Graphics/RHI/ComputeState.h>
#include <Methane/Graphics/RHI/Program.h>
#include <Methane/Graphics/RHI/Shader.h>
#include <Methane/Graphics/RHI/Buffer.h>
#include <Methane/Graphics/RHI/Texture.h>
#include <Methane/Graphics/RHI/Sampler.h>
#include <Methane/Graphics/RHI/ObjectRegistry.h>

#include <taskflow/taskflow.hpp>
#include <magic_enum/magic_enum.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

static tf::Executor g_parallel_executor;

static const Platform::AppEnvironment test_app_env{ nullptr };
static const Rhi::RenderContextSettings render_context_settings = Test::GetRenderContextSettings();

TEST_CASE("RHI Render Context Basic Functions", "[rhi][basic][context]")
{
    SECTION("Context Construction")
    {
        Rhi::RenderContext render_context;
        REQUIRE_NOTHROW(render_context = Rhi::RenderContext(test_app_env, GetTestDevice(),
                                                            g_parallel_executor, render_context_settings));
        REQUIRE(render_context.IsInitialized());
        CHECK(render_context.GetInterfacePtr());
        CHECK(render_context.GetSettings() == render_context_settings);
        CHECK(render_context.GetOptions() == render_context_settings.options_mask);
        CHECK(render_context.GetName() == "");
        CHECK(render_context.GetDevice() == GetTestDevice());
        CHECK(std::addressof(render_context.GetParallelExecutor()) == std::addressof(g_parallel_executor));
    }

    SECTION("Object Destroyed Callback")
    {
        auto render_context_ptr = std::make_unique<Rhi::RenderContext>(test_app_env, GetTestDevice(),
                                                                       g_parallel_executor, render_context_settings);
        ObjectCallbackTester object_callback_tester(*render_context_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        render_context_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    const Rhi::RenderContext render_context(test_app_env, GetTestDevice(), g_parallel_executor, render_context_settings);

    SECTION("Object Name Setup")
    {
        CHECK(render_context.SetName("My Render Context"));
        CHECK(render_context.GetName() == "My Render Context");
    }

    SECTION("Object Name Change Callback")
    {
        CHECK(render_context.SetName("My Render Context"));
        ObjectCallbackTester object_callback_tester(render_context);
        CHECK(render_context.SetName("Our Render Context"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our Render Context");
        CHECK(object_callback_tester.GetOldObjectName() == "My Render Context");
    }

    SECTION("Object Name Set Unchanged")
    {
        CHECK(render_context.SetName("My Render Context"));
        ObjectCallbackTester object_callback_tester(render_context);
        CHECK_FALSE(render_context.SetName("My Render Context"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Add to Objects Registry")
    {
        render_context.SetName("Render Context");
        Rhi::ObjectRegistry registry = render_context.GetObjectRegistry();
        registry.AddGraphicsObject(render_context);
        const auto registered_context = registry.GetGraphicsObject<Rhi::RenderContext>("Render Context");
        REQUIRE(registered_context.IsInitialized());
        CHECK(&registered_context.GetInterface() == &render_context.GetInterface());
    }

    SECTION("Context Reset")
    {
        ContextCallbackTester context_callback_tester(render_context);
        REQUIRE_NOTHROW(render_context.Reset());
        CHECK(context_callback_tester.IsContextReleased());
        CHECK_FALSE(context_callback_tester.IsContextUploadingResources());
        CHECK(context_callback_tester.IsContextInitialized());
    }

    SECTION("Context Reset with Device")
    {
        ContextCallbackTester context_callback_tester(render_context);
        const Rhi::Device new_device = Rhi::System::Get().UpdateGpuDevices().at(0);
        REQUIRE_NOTHROW(render_context.Reset(new_device));
        CHECK(context_callback_tester.IsContextReleased());
        CHECK_FALSE(context_callback_tester.IsContextUploadingResources());
        CHECK(context_callback_tester.IsContextInitialized());
        CHECK(render_context.GetDevice() == new_device);
    }

    SECTION("Context Upload Command Kit")
    {
        Rhi::CommandKit upload_cmd_kit;
        REQUIRE_NOTHROW(upload_cmd_kit = render_context.GetUploadCommandKit());
        REQUIRE(upload_cmd_kit.IsInitialized());
        CHECK(upload_cmd_kit.GetListType() == Rhi::CommandListType::Transfer);
    }

    SECTION("Context Render Command Kit")
    {
        Rhi::CommandKit render_cmd_kit;
        REQUIRE_NOTHROW(render_cmd_kit = render_context.GetRenderCommandKit());
        REQUIRE(render_cmd_kit.IsInitialized());
        CHECK(render_cmd_kit.GetListType() == Rhi::CommandListType::Render);
    }

    SECTION("Context Compute Command Kit")
    {
        Rhi::CommandKit render_cmd_kit;
        REQUIRE_NOTHROW(render_cmd_kit = render_context.GetComputeCommandKit());
        REQUIRE(render_cmd_kit.IsInitialized());
        CHECK(render_cmd_kit.GetListType() == Rhi::CommandListType::Compute);
    }

    SECTION("Context Default Command Kits")
    {
        const std::array cmd_list_types {
            Rhi::CommandListType::Render,
            Rhi::CommandListType::Compute,
            Rhi::CommandListType::Transfer
        };
        for(Rhi::CommandListType cmd_list_type : cmd_list_types)
        {
            Rhi::CommandKit default_cmd_kit = render_context.GetDefaultCommandKit(cmd_list_type);
            CHECK(default_cmd_kit.IsInitialized());
            CHECK(default_cmd_kit.GetListType() == cmd_list_type);
        }
    }

    SECTION("Context Upload Resources")
    {
        Rhi::TransferCommandList transfer_cmd_list = render_context.GetUploadCommandKit().GetTransferListForEncoding();
        CHECK(transfer_cmd_list.GetState() == Rhi::CommandListState::Encoding);
        CHECK_NOTHROW(render_context.UploadResources());
        CHECK(transfer_cmd_list.GetState() == Rhi::CommandListState::Executing);
    }

    SECTION("Context Upload Resources Deferred")
    {
        ContextCallbackTester context_callback_tester(render_context);
        Rhi::TransferCommandList transfer_cmd_list = render_context.GetUploadCommandKit().GetTransferListForEncoding();
        CHECK(transfer_cmd_list.GetState() == Rhi::CommandListState::Encoding);
        CHECK_NOTHROW(render_context.RequestDeferredAction(Rhi::ContextDeferredAction::UploadResources));
        CHECK_NOTHROW(render_context.WaitForGpu(Rhi::ContextWaitFor::FramePresented));
        CHECK(context_callback_tester.IsContextUploadingResources());
        CHECK(transfer_cmd_list.GetState() == Rhi::CommandListState::Executing);
    }

    SECTION("Context Complete Initialization")
    {
        ContextCallbackTester context_callback_tester(render_context);
        REQUIRE_NOTHROW(render_context.CompleteInitialization());
        CHECK(context_callback_tester.IsContextUploadingResources());
        CHECK_FALSE(render_context.IsCompletingInitialization());
    }

    SECTION("Context Complete Initialization Deferred")
    {
        ContextCallbackTester context_callback_tester(render_context);
        Rhi::TransferCommandList transfer_cmd_list = render_context.GetUploadCommandKit().GetTransferListForEncoding();
        CHECK(transfer_cmd_list.GetState() == Rhi::CommandListState::Encoding);
        CHECK_NOTHROW(render_context.RequestDeferredAction(Rhi::ContextDeferredAction::CompleteInitialization));
        CHECK_NOTHROW(render_context.WaitForGpu(Rhi::ContextWaitFor::RenderComplete));
        CHECK(transfer_cmd_list.GetState() == Rhi::CommandListState::Executing);
    }
}

TEST_CASE("RHI Render Context Rendering Functions", "[rhi][render][context]")
{
    const Rhi::RenderContext render_context(test_app_env, GetTestDevice(), g_parallel_executor, render_context_settings);

    SECTION("Get Context Settings")
    {
        CHECK(render_context.GetSettings() == render_context_settings);
    }

    SECTION("Context Ready to Render")
    {
        CHECK(render_context.ReadyToRender());
    }

    SECTION("Context Resize")
    {
        const FrameSize new_frame_size(800, 600);
        CHECK_FALSE(render_context.GetSettings().frame_size == new_frame_size);
        CHECK_NOTHROW(render_context.Resize(new_frame_size));
        CHECK(render_context.GetSettings().frame_size == new_frame_size);
    }

    SECTION("Set Context Full-Screen")
    {
        const bool new_full_screen = !render_context.GetSettings().is_full_screen;
        CHECK(render_context.SetFullScreen(new_full_screen));
        CHECK(render_context.GetSettings().is_full_screen == new_full_screen);
    }

    SECTION("Get Context VSync Enabled")
    {
        const bool new_vsync_enabled = !render_context.GetSettings().vsync_enabled;
        CHECK(render_context.SetVSyncEnabled(new_vsync_enabled));
        CHECK(render_context.GetSettings().vsync_enabled == new_vsync_enabled);
    }

    SECTION("Context Present")
    {
        const Data::Index frame_index = render_context.GetFrameIndex();
        CHECK_NOTHROW(render_context.Present());
        CHECK(render_context.GetFrameIndex() == frame_index + 1);
    }

    SECTION("Get Context App View")
    {
        CHECK_NOTHROW(render_context.GetAppView());
    }

    SECTION("Set Context Frame Buffers Count")
    {
        const uint32_t new_frame_buffers_count = render_context_settings.frame_buffers_count + 1U;
        CHECK(render_context.SetFrameBuffersCount(new_frame_buffers_count));
        CHECK(render_context.GetSettings().frame_buffers_count == new_frame_buffers_count);
    }

    SECTION("Get Context Frame Buffer Index")
    {
        CHECK(render_context.GetFrameBufferIndex() == 0U);
        CHECK_NOTHROW(render_context.Present());
        CHECK(render_context.GetFrameBufferIndex() == 1U);
        CHECK_NOTHROW(render_context.Present());
        CHECK(render_context.GetFrameBufferIndex() == 0U);
        CHECK_NOTHROW(render_context.Present());
        CHECK(render_context.GetFrameBufferIndex() == 1U);
    }

    SECTION("Get Context Frame Index")
    {
        CHECK(render_context.GetFrameIndex() == 0U);
        CHECK_NOTHROW(render_context.Present());
        CHECK(render_context.GetFrameIndex() == 1U);
        CHECK_NOTHROW(render_context.Present());
        CHECK(render_context.GetFrameIndex() == 2U);
        CHECK_NOTHROW(render_context.Present());
        CHECK(render_context.GetFrameIndex() == 3U);
    }

    SECTION("Read Context FPS Counter")
    {
        CHECK(render_context.GetFpsCounter().GetAveragedTimingsCount() == 0U);
        CHECK(render_context.GetFpsCounter().GetFramesPerSecond() == 0U);

        for(size_t i = 0; i < 60; ++i)
        {
            render_context.Present();
            std::this_thread::sleep_for(std::chrono::milliseconds(17));
        }

        Data::FrameTiming avg_frame_timing;
        CHECK_NOTHROW(avg_frame_timing = render_context.GetFpsCounter().GetAverageFrameTiming());
        CHECK(avg_frame_timing.GetTotalTimeMSec() >= 16.0);
        CHECK(avg_frame_timing.GetPresentTimeMSec() <= 1.0);
        CHECK(render_context.GetFpsCounter().GetFramesPerSecond() <= 60U);
    }
}

TEST_CASE("RHI Render Context Factory", "[rhi][render][context][factory]")
{
    const Rhi::RenderContext render_context(Platform::AppEnvironment{}, GetTestDevice(), g_parallel_executor, {});

    SECTION("Can Create Render Command Queue")
    {
        Rhi::CommandQueue command_queue;
        REQUIRE_NOTHROW(command_queue = render_context.CreateCommandQueue(Rhi::CommandListType::Render));
        REQUIRE(command_queue.IsInitialized());
        CHECK(command_queue.GetCommandListType() == Rhi::CommandListType::Render);
    }

    SECTION("Can Create Compute Command Queue")
    {
        Rhi::CommandQueue command_queue;
        REQUIRE_NOTHROW(command_queue = render_context.CreateCommandQueue(Rhi::CommandListType::Compute));
        REQUIRE(command_queue.IsInitialized());
        CHECK(command_queue.GetCommandListType() == Rhi::CommandListType::Compute);
    }

    SECTION("Can Create Transfer Command Queue")
    {
        Rhi::CommandQueue command_queue;
        REQUIRE_NOTHROW(command_queue = render_context.CreateCommandQueue(Rhi::CommandListType::Transfer));
        REQUIRE(command_queue.IsInitialized());
        CHECK(command_queue.GetCommandListType() == Rhi::CommandListType::Transfer);
    }

    SECTION("Can Create Render Command Kit")
    {
        Rhi::CommandKit command_kit;
        REQUIRE_NOTHROW(command_kit = render_context.CreateCommandKit(Rhi::CommandListType::Render));
        REQUIRE(command_kit.IsInitialized());
        CHECK(command_kit.GetListType() == Rhi::CommandListType::Render);
    }

    SECTION("Can Create Compute Command Kit")
    {
        Rhi::CommandKit command_kit;
        REQUIRE_NOTHROW(command_kit = render_context.CreateCommandKit(Rhi::CommandListType::Compute));
        REQUIRE(command_kit.IsInitialized());
        CHECK(command_kit.GetListType() == Rhi::CommandListType::Compute);
    }

    SECTION("Can Create Transfer Command Kit")
    {
        Rhi::CommandKit command_kit;
        REQUIRE_NOTHROW(command_kit = render_context.CreateCommandKit(Rhi::CommandListType::Transfer));
        REQUIRE(command_kit.IsInitialized());
        CHECK(command_kit.GetListType() == Rhi::CommandListType::Transfer);
    }

    SECTION("Can Create Shader")
    {
        const Rhi::ShaderSettings shader_settings{ Data::ShaderProvider::Get(), { "Shader", "Main" } };
        Rhi::Shader shader;
        REQUIRE_NOTHROW(shader = render_context.CreateShader(Rhi::ShaderType::Pixel, shader_settings));
        REQUIRE(shader.IsInitialized());
        CHECK(shader.GetType() == Rhi::ShaderType::Pixel);
        CHECK(shader.GetSettings().entry_function == shader_settings.entry_function);
    }

    SECTION("Can Create Program")
    {
        Rhi::Program program;
        REQUIRE_NOTHROW(program = render_context.CreateProgram({
            { { Rhi::ShaderType::Pixel, { Data::ShaderProvider::Get(), { "Shader", "Main" } } } },
        }));
        REQUIRE(program.IsInitialized());
        CHECK(program.GetSettings().shaders.size() == 1);
        CHECK(program.GetShader(Rhi::ShaderType::Pixel).GetSettings().entry_function == Rhi::ShaderEntryFunction{ "Shader", "Main" });
    }

    SECTION("Can Create Render Pattern")
    {
        Rhi::RenderPatternSettings render_pattern_settings = Test::GetRenderPatternSettings();
        Rhi::RenderPattern render_pattern;
        REQUIRE_NOTHROW(render_pattern = render_context.CreateRenderPattern(render_pattern_settings));
        REQUIRE(render_pattern.IsInitialized());
        CHECK(render_pattern.GetSettings() == render_pattern_settings);
    }

    SECTION("Can Create Render State")
    {
        Rhi::RenderPattern render_pattern = render_context.CreateRenderPattern(Test::GetRenderPatternSettings());
        Rhi::RenderStateSettingsImpl render_state_settings = Test::GetRenderStateSettings(render_context, render_pattern);
        Rhi::RenderState render_state;
        REQUIRE_NOTHROW(render_state = render_context.CreateRenderState(render_state_settings));
        REQUIRE(render_state.IsInitialized());
        CHECK(render_state.GetSettings() == Rhi::RenderStateSettingsImpl::Convert(render_state_settings));
    }

    SECTION("Can Create Compute State")
    {
        const Rhi::ComputeStateSettingsImpl& compute_state_settings{
            render_context.CreateProgram({
                { { Rhi::ShaderType::Compute, { Data::ShaderProvider::Get(), { "Shader", "Main" } } } },
            }),
            Rhi::ThreadGroupSize(16, 16, 1)
        };
        Rhi::ComputeState compute_state;
        REQUIRE_NOTHROW(compute_state = render_context.CreateComputeState(compute_state_settings));
        REQUIRE(compute_state.IsInitialized());
        CHECK(compute_state.GetSettings() == Rhi::ComputeStateSettingsImpl::Convert(compute_state_settings));
    }

    SECTION("Can Create Buffer")
    {
        const Rhi::BufferSettings buffer_settings = Rhi::BufferSettings::ForConstantBuffer(42);
        Rhi::Buffer buffer;
        REQUIRE_NOTHROW(buffer = render_context.CreateBuffer(buffer_settings));
        REQUIRE(buffer.IsInitialized());
        CHECK(buffer.GetSettings().type == Rhi::BufferType::Constant);
        CHECK(buffer.GetSettings().size == buffer_settings.size);
    }

    SECTION("Can Create Texture")
    {
        const Rhi::TextureSettings buffer_settings = Rhi::TextureSettings::ForImage(Dimensions(640, 480), 2, PixelFormat::RGBA8Unorm_sRGB, true);
        Rhi::Texture texture;
        REQUIRE_NOTHROW(texture = render_context.CreateTexture(buffer_settings));
        REQUIRE(texture.IsInitialized());
        CHECK(texture.GetSettings().type == Rhi::TextureType::Image);
        CHECK(texture.GetSettings().dimension_type == Rhi::TextureDimensionType::Tex2DArray);
        CHECK(texture.GetSettings().array_length == 2);
        CHECK(texture.GetSettings().dimensions == Dimensions(640, 480));
        CHECK(texture.GetSettings().pixel_format == PixelFormat::RGBA8Unorm_sRGB);
        CHECK(texture.GetSettings().mipmapped);
    }

    SECTION("Can Create Sampler")
    {
        Rhi::Sampler sampler;
        REQUIRE_NOTHROW(sampler = render_context.CreateSampler(
            Rhi::SamplerSettings
            {
                .filter  = rhi::SamplerFilter  { rhi::SamplerFilter::MinMag::Linear },
                .address = rhi::SamplerAddress { rhi::SamplerAddress::Mode::ClampToEdge }
            }));
        REQUIRE(sampler.IsInitialized());
        CHECK(sampler.GetSettings().filter.min == Rhi::SamplerFilter::MinMag::Linear);
        CHECK(sampler.GetSettings().filter.mag == Rhi::SamplerFilter::MinMag::Linear);
        CHECK(sampler.GetSettings().address.r == Rhi::SamplerAddress::Mode::ClampToEdge);
        CHECK(sampler.GetSettings().address.s == Rhi::SamplerAddress::Mode::ClampToEdge);
        CHECK(sampler.GetSettings().address.t == Rhi::SamplerAddress::Mode::ClampToEdge);
    }

    SECTION("Can Get Object Registry")
    {
        CHECK_FALSE(render_context.GetObjectRegistry().HasGraphicsObject("Something"));
    }

    SECTION("Can Get Parallel Executor")
    {
        tf::Executor* executor_ptr = nullptr;
        REQUIRE_NOTHROW(executor_ptr = &render_context.GetParallelExecutor());
        REQUIRE(executor_ptr);
        CHECK(executor_ptr->num_workers() > 0);
    }
}
