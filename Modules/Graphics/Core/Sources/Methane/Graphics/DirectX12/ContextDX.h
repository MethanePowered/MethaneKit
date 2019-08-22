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
#include <Methane/Graphics/CommandQueueBase.h>

#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <d3dx12.h>

#include <vector>

namespace Methane
{
namespace Graphics
{

namespace wrl = Microsoft::WRL;

struct AppEnvironment;
class CommandQueueDX;
class DeviceBase;
class DeviceDX;

class ContextDX final : public ContextBase
{
public:
    ContextDX(const Platform::AppEnvironment& env, const Data::Provider& data_provider, DeviceBase& device, const Settings& settings);
    ~ContextDX() override;

    // Context interface
    bool ReadyToRender() const override { return true; }
    void WaitForGpu(WaitFor wait_for) override;
    void Resize(const FrameSize& frame_size) override;
    void Reset(Device& device) override;
    void Present() override;
    Platform::AppView GetAppView() const override { return { nullptr }; }

    // ContextBase interface
    void OnCommandQueueCompleted(CommandQueue& cmd_list, uint32_t frame_index) override;

    // Object interface
    void SetName(const std::string& name) override;

    const DeviceDX& GetDeviceDX() const;
    const wrl::ComPtr<IDXGISwapChain3>& GetNativeSwapChain() const { return m_cp_swap_chain; }

protected:
    struct FrameFence
    {
        wrl::ComPtr<ID3D12Fence> cp_fence;
        uint64_t                 value = 0;
        uint32_t                 frame = 0;
    };

    using FrameFences = std::vector<FrameFence>;

    inline const FrameFence& GetCurrentFrameFence() const                     { return m_frame_fences[m_frame_buffer_index]; }
    inline FrameFence&       GetCurrentFrameFence()                           { return m_frame_fences[m_frame_buffer_index]; }
    inline uint32_t          GetPresentVSyncInterval() const                  { return m_settings.vsync_enabled ? 1 : 0; }
    void                     SignalFence(const FrameFence& frame_fence, CommandQueueDX& dx_command_queue);
    void                     WaitFence(FrameFence& frame_fence, bool increment_value);

    void            Initialize(const DeviceDX& device);
    CommandQueueDX& DefaultCommandQueueDX();

    const Platform::AppEnvironment m_platform_env;
    wrl::ComPtr<IDXGISwapChain3>   m_cp_swap_chain;
    FrameFences                    m_frame_fences;
    FrameFence                     m_upload_fence;
    HANDLE                         m_fence_event = nullptr;
};

} // namespace Graphics
} // namespace Methane
