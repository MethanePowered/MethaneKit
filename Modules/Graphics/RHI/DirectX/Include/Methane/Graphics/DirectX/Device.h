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

FILE: Methane/Graphics/Metal/Device.h
DirectX 12 implementation of the device interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/Device.h>

#include <wrl.h>
#include <dxgi1_6.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>

#include <optional>

// NOTE: Adapters change handling breaks many frame capture tools, like VS or RenderDoc
//#define ADAPTERS_CHANGE_HANDLING

namespace Methane::Graphics::DirectX
{

namespace wrl = Microsoft::WRL;

class Device final : public Base::Device
{
public:
    static Rhi::DeviceFeatures GetSupportedFeatures(const wrl::ComPtr<IDXGIAdapter>& cp_adapter, D3D_FEATURE_LEVEL feature_level);

    Device(const wrl::ComPtr<IDXGIAdapter>& cp_adapter, D3D_FEATURE_LEVEL feature_level, const Capabilities& capabilities);

    // IObject interface
    bool SetName(const std::string& name) override;

    using NativeFeatureOptions5 = std::optional<D3D12_FEATURE_DATA_D3D12_OPTIONS5>;
    const NativeFeatureOptions5&        GetNativeFeatureOptions5() const { return m_feature_options_5; }
    const wrl::ComPtr<IDXGIAdapter>&    GetNativeAdapter() const         { return m_cp_adapter; }
    const wrl::ComPtr<ID3D12Device>&    GetNativeDevice() const;
    void ReleaseNativeDevice();

private:
    const wrl::ComPtr<IDXGIAdapter>     m_cp_adapter;
    const D3D_FEATURE_LEVEL             m_feature_level;
    mutable NativeFeatureOptions5       m_feature_options_5;
    mutable wrl::ComPtr<ID3D12Device>   m_cp_device;
};

class System final // NOSONAR - custom destructor is required
    : public Base::System
{
public:
    [[nodiscard]] static System& Get() { return static_cast<System&>(ISystem::Get()); }

    System();
    System(const System&) = delete;
    System(System&&) = delete;
    ~System() override;

    System& operator=(const System&) = delete;
    System& operator=(System&&) = delete;

    // ISystem interface
    void  CheckForChanges() override;
    const Ptrs<Rhi::IDevice>& UpdateGpuDevices(const Platform::AppEnvironment& app_env, const Rhi::DeviceCaps& required_device_caps) override;
    const Ptrs<Rhi::IDevice>& UpdateGpuDevices(const Rhi::DeviceCaps& required_device_caps) override;

    [[nodiscard]] const wrl::ComPtr<IDXGIFactory5>& GetNativeFactory() const noexcept { return m_cp_factory; }
    void ReportLiveObjects() const noexcept;

private:
    void Initialize();
    void AddDevice(const wrl::ComPtr<IDXGIAdapter>& cp_adapter, D3D_FEATURE_LEVEL feature_level);

#ifdef ADAPTERS_CHANGE_HANDLING
    void RegisterAdapterChangeEvent();
    void UnregisterAdapterChangeEvent();

    HANDLE m_adapter_change_event = NULL;
    DWORD  m_adapter_change_registration_cookie = 0;
#endif

    wrl::ComPtr<IDXGIFactory5> m_cp_factory;
};

} // namespace Methane::Graphics::DirectX