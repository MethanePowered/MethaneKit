/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/DirectX12/ContextDX.cpp
DirectX 12 implementation of the context interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ContextBase.h>

#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <d3dx12.h>

#include <vector>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

struct AppEnvironment;
class CommandQueueDX;
class DeviceBase;
class DeviceDX;

class ContextDX final : public ContextBase
{
public:
    ContextDX(const Platform::AppEnvironment& env, DeviceBase& device, const Settings& settings);
    ~ContextDX() override;

    // Context interface
    bool ReadyToRender() const override { return true; }
    void WaitForGpu(WaitFor wait_for) override;
    void Resize(const FrameSize& frame_size) override;
    void Present() override;
    Platform::AppView GetAppView() const override { return { nullptr }; }
    float GetContentScalingFactor() const override { return 1.f; }

    // ContextBase interface
    void OnCommandQueueCompleted(CommandQueue& cmd_list, uint32_t frame_index) override;

    // Object interface
    void SetName(const std::string& name) override;

    const DeviceDX& GetDeviceDX() const;
    CommandQueueDX& GetUploadCommandQueueDX();
    CommandQueueDX& GetRenderCommandQueueDX();

    const wrl::ComPtr<IDXGISwapChain3>& GetNativeSwapChain() const { return m_cp_swap_chain; }

protected:
    class FenceDX
    {
    public:
        using Ptr = std::unique_ptr<FenceDX>;
        FenceDX(CommandQueueDX& command_queue, uint32_t frame = static_cast<uint32_t>(-1));
        ~FenceDX();

        void Signal();
        void Wait();
        void Flush();

        uint32_t           GetFrame() const { return m_frame; }
        const std::string& GetName() const { return m_name; }
        void               SetName(const std::string& name);

    private:
        CommandQueueDX&          m_command_queue;
        const uint32_t           m_frame = 0;
        uint64_t                 m_value = 0;
        wrl::ComPtr<ID3D12Fence> m_cp_fence;
        HANDLE                   m_event = nullptr;
        std::string              m_name;
    };

    FenceDX&             GetCurrentFrameFence();
    inline FenceDX::Ptr& GetCurrentFrameFencePtr()       { return m_frame_fences[m_frame_buffer_index]; }
    inline uint32_t      GetPresentVSyncInterval() const { return m_settings.vsync_enabled ? 1 : 0; }

    // ContextBase overrides
    void Release() override;
    void Initialize(Device& device, bool deferred_heap_allocation) override;

    const Platform::AppEnvironment m_platform_env;
    wrl::ComPtr<IDXGISwapChain3>   m_cp_swap_chain;
    std::vector<FenceDX::Ptr>      m_frame_fences;
    FenceDX::Ptr                   m_sp_render_fence;
    FenceDX::Ptr                   m_sp_upload_fence;
};

} // namespace Methane::Graphics
