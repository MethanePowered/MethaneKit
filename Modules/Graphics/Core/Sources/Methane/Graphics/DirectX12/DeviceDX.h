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

FILE: Methane/Graphics/Metal/DeviceDX.h
DirectX 12 implementation of the device interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/DeviceBase.h>

#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dx12.h>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

class DeviceDX final : public DeviceBase
{
public:
    static Device::Feature::Mask GetSupportedFeatures(const wrl::ComPtr<IDXGIAdapter>& cp_adapter, D3D_FEATURE_LEVEL feature_level);

    DeviceDX(const wrl::ComPtr<IDXGIAdapter>& cp_adapter, D3D_FEATURE_LEVEL feature_level);
    ~DeviceDX() override;

    // Object interface
    void SetName(const std::string& name) override;

    const wrl::ComPtr<IDXGIAdapter>& GetNativeAdapter() const { return m_cp_adapter; }
    const wrl::ComPtr<ID3D12Device>& GetNativeDevice() const;
    void ReleaseNativeDevice();

protected:
    const wrl::ComPtr<IDXGIAdapter>   m_cp_adapter;
    const D3D_FEATURE_LEVEL           m_feature_level;
    mutable wrl::ComPtr<ID3D12Device> m_cp_device;
};

class SystemDX final : public SystemBase
{
public:
    static SystemDX& Get() { return static_cast<SystemDX&>(System::Get()); }

    SystemDX();
    ~SystemDX() override;

    // System interface
    void           CheckForChanges() override;
    const Devices& UpdateGpuDevices(Device::Feature::Mask supported_features) override;

    const wrl::ComPtr<IDXGIFactory5>& GetNativeFactory() { return m_cp_factory; }
    void ReportLiveObjects();

private:
    void Initialize();
    void RegisterAdapterChangeEvent();
    void UnregisterAdapterChangeEvent();
    void AddDevice(const wrl::ComPtr<IDXGIAdapter>& cp_adapter, D3D_FEATURE_LEVEL feature_level);

    wrl::ComPtr<IDXGIFactory5> m_cp_factory;
    HANDLE                     m_adapter_change_event = NULL;
    DWORD                      m_adapter_change_registration_cookie = 0;
};

} // namespace Methane::Graphics
