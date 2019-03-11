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

class ContextDX final : public ContextBase
{
public:
    ContextDX(const Platform::AppEnvironment& env, const Data::Provider& data_provider, const Settings& settings);
    virtual ~ContextDX() override;

    // Context interface
    virtual bool ReadyToRender() const override { return true; }
    virtual void WaitForGpu(WaitFor wait_for) override;
    virtual void Resize(const FrameSize& frame_size) override;
    virtual void Present() override;
    virtual Platform::AppView GetAppView() const override { return { nullptr }; }

    // ContextBase interface
    virtual void OnCommandQueueCompleted(CommandQueue& cmd_list, uint32_t frame_index) override;

    // Object interface
    virtual void SetName(const std::string& name) override;

    const wrl::ComPtr<ID3D12Device>&    GetNativeDevice() const    { return m_cp_device; }
    const wrl::ComPtr<IDXGISwapChain3>& GetNativeSwapChain() const { return m_cp_swap_chain; }

protected:
    CommandQueueDX& DefaultCommandQueueDX();

    inline UINT64 GetCurrentFenceValue() const             { return m_fence_values[m_frame_buffer_index]; }
    inline void SetCurrentFenceValue(uint64_t fence_value) { m_fence_values[m_frame_buffer_index] = fence_value; }

    const uint32_t               m_present_sync_interval;
    const uint32_t               m_present_flags;
    wrl::ComPtr<ID3D12Device>    m_cp_device;
    wrl::ComPtr<IDXGISwapChain3> m_cp_swap_chain;
    wrl::ComPtr<ID3D12Fence>     m_cp_fence;
    HANDLE                       m_fence_event = nullptr;
    std::vector<uint64_t>        m_fence_values;
};

} // namespace Graphics
} // namespace Methane
