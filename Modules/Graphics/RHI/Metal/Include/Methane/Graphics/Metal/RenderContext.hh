/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/RenderContext.hh
Metal implementation of the render context interface.

******************************************************************************/

#pragma once

#include "Context.hpp"

#include <Methane/Graphics/Base/RenderContext.h>
#include <Methane/TracyGpu.hpp>
#include <Methane/Platform/AppView.h>

#import <Metal/Metal.h>

namespace Methane::Graphics::Metal
{

class RenderContext final
    : public Context<Base::RenderContext>
{
public:
    RenderContext(const Platform::AppEnvironment& env, Base::Device& device, tf::Executor& parallel_executor, const Settings& settings);
    ~RenderContext() override;

    // IContext interface
    [[nodiscard]] Ptr<Rhi::ICommandQueue> CreateCommandQueue(Rhi::CommandListType type) const override;
    [[nodiscard]] Ptr<Rhi::IShader> CreateShader(Rhi::ShaderType type, const Rhi::ShaderSettings& settings) const override;
    [[nodiscard]] Ptr<Rhi::IProgram> CreateProgram(const Rhi::ProgramSettings& settings) const override;
    [[nodiscard]] Ptr<Rhi::IBuffer> CreateBuffer(const Rhi::BufferSettings& settings) const override;
    [[nodiscard]] Ptr<Rhi::ITexture> CreateTexture(const Rhi::TextureSettings& settings) const override;
    [[nodiscard]] Ptr<Rhi::ISampler> CreateSampler(const Rhi::SamplerSettings& settings) const override;
    void  WaitForGpu(WaitFor wait_for) override;

    // IRenderContext interface
    [[nodiscard]] Ptr<Rhi::IRenderState> CreateRenderState(const Rhi::RenderStateSettings& settings) const override;
    [[nodiscard]] Ptr<Rhi::IRenderPattern> CreateRenderPattern(const Rhi::RenderPatternSettings& settings) override;
    bool     ReadyToRender() const override;
    void     Resize(const FrameSize& frame_size) override;
    void     Present() override;
    bool     SetVSyncEnabled(bool vsync_enabled) override;
    bool     SetFrameBuffersCount(uint32_t frame_buffers_count) override;
    Platform::AppView GetAppView() const override { return { m_app_view }; }

    // Base::Context overrides
    void Initialize(Base::Device& device, bool is_callback_emitted = true) override;
    void Release() override;

    id<CAMetalDrawable> GetNativeDrawable() const { return m_app_view.currentDrawable; }

    void OnGpuExecutionCompleted();

private:
    void BeginFrameCaptureScope();
    void EndFrameCaptureScope();
    void Capture(const id<MTLCaptureScope>& capture_scope);

    AppViewMetal*        m_app_view;
    id<MTLCaptureScope>  m_frame_capture_scope;
    bool                 m_frame_capture_scope_begun = false;
    dispatch_semaphore_t m_dispatch_semaphore;
};

} // namespace Methane::Graphics::Metal
