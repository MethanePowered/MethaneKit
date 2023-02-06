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

FILE: Methane/Graphics/DirectX/RenderContext.cpp
DirectX 12 implementation of the render context interface.

******************************************************************************/

#pragma once

#include "Context.hpp"

#include <Methane/Graphics/Base/RenderContext.h>

#include <dxgi1_4.h>
#include <vector>

namespace Methane::Graphics::DirectX
{

class RenderContext final // NOSONAR - manual destructor is required, inheritance hierarchy is greater than 5
    : public Context<Base::RenderContext>
{
public:
    RenderContext(const Platform::AppEnvironment& env, Base::Device& device, tf::Executor& parallel_executor,
                  const Rhi::RenderContextSettings& settings);

    // IContext interface
    [[nodiscard]] Ptr<Rhi::ICommandQueue> CreateCommandQueue(Rhi::CommandListType type) const override;
    [[nodiscard]] Ptr<Rhi::IShader> CreateShader(Rhi::ShaderType type, const Rhi::ShaderSettings& settings) const override;
    [[nodiscard]] Ptr<Rhi::IProgram>      CreateProgram(const Rhi::ProgramSettings& settings) const override;
    [[nodiscard]] Ptr<Rhi::IBuffer>       CreateBuffer(const Rhi::BufferSettings& settings) const override;
    [[nodiscard]] Ptr<Rhi::ITexture>      CreateTexture(const Rhi::TextureSettings& settings) const override;
    [[nodiscard]] Ptr<Rhi::ISampler>      CreateSampler(const Rhi::SamplerSettings& settings) const override;
    void WaitForGpu(WaitFor wait_for) override;

    // IRenderContext interface
    [[nodiscard]] Ptr<Rhi::IRenderState>   CreateRenderState(const Rhi::RenderStateSettings& settings) const override;
    [[nodiscard]] Ptr<Rhi::IRenderPattern> CreateRenderPattern(const Rhi::RenderPatternSettings& settings) override;
    bool ReadyToRender() const override { return true; }
    void Resize(const FrameSize& frame_size) override;
    void Present() override;
    Platform::AppView GetAppView() const override { return { nullptr }; }

    // Base::Context interface
    void Initialize(Base::Device& device, bool is_callback_emitted = true) override;
    void Release() override;

    const wrl::ComPtr<IDXGISwapChain3>& GetNativeSwapChain() const { return m_cp_swap_chain; }

protected:
    // Base::RenderContext overrides
    uint32_t GetNextFrameBufferIndex() override;

private:
    uint32_t GetPresentVSyncInterval() const noexcept { return GetSettings().vsync_enabled ? 1 : 0; }
    uint32_t GetPresentFlags() const noexcept         { return !GetSettings().vsync_enabled && m_is_tearing_supported ? DXGI_PRESENT_ALLOW_TEARING : 0; }

    void WaitForSwapChainLatency();

    const Platform::AppEnvironment m_platform_env;
    wrl::ComPtr<IDXGISwapChain3>   m_cp_swap_chain;
    HANDLE                         m_frame_latency_waitable_object {};
    bool                           m_is_tearing_supported = false;
};

} // namespace Methane::Graphics::DirectX
