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
#include <dxgi1_4.h>
#include <d3d12.h>
#include <d3dx12.h>

namespace Methane
{
namespace Graphics
{

namespace wrl = Microsoft::WRL;

class DeviceDX final : public DeviceBase
{
public:
    static Device::Feature::Mask GetSupportedFeatures(const wrl::ComPtr<IDXGIAdapter>& cp_adapter, D3D_FEATURE_LEVEL feature_level);

    DeviceDX(const wrl::ComPtr<IDXGIAdapter>& cp_adapter, D3D_FEATURE_LEVEL feature_level);
    ~DeviceDX() override;

    const wrl::ComPtr<ID3D12Device>& GetNativeDevice() const { return m_cp_device; }

protected:
    wrl::ComPtr<IDXGIAdapter> m_cp_adapter;
    mutable wrl::ComPtr<ID3D12Device> m_cp_device;
};

class SystemDX final : public SystemBase
{
public:
    static SystemDX& Get() { return static_cast<SystemDX&>(System::Get()); }

    SystemDX();
    ~SystemDX() override;
    
    const Devices& UpdateGpuDevices(Device::Feature::Mask supported_features) override;

    const wrl::ComPtr<IDXGIFactory4>& GetNativeFactory() { return m_cp_factory; }
    
private:
    void AddDevice(const wrl::ComPtr<IDXGIAdapter>& cp_adapter, D3D_FEATURE_LEVEL feature_level);

    wrl::ComPtr<IDXGIFactory4> m_cp_factory;
};

} // namespace Graphics
} // namespace Methane
