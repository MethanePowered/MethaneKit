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

FILE: Methane/Graphics/Metal/DeviceDX.cpp
DirectX 12 implementation of the device interface.

******************************************************************************/

#include "DeviceDX.h"

#include <Methane/Graphics/Instrumentation.h>
#include <Methane/Graphics/Windows/Helpers.h>

#include <nowide/convert.hpp>
#include <algorithm>
#include <cassert>

using namespace Methane::Graphics;
using namespace Methane;

Device::Feature::Mask DeviceDX::GetSupportedFeatures(const wrl::ComPtr<IDXGIAdapter>& cp_adapter, D3D_FEATURE_LEVEL feature_level)
{
    ITT_FUNCTION_TASK();
    Device::Feature::Mask supported_featues = Device::Feature::Value::BasicRendering;
    cp_adapter;
    feature_level;
    return supported_featues;
}

std::string GetAdapterName(const wrl::ComPtr<IDXGIAdapter>& cp_adapter)
{
    ITT_FUNCTION_TASK();
    assert(!!cp_adapter);

    DXGI_ADAPTER_DESC desc;
    cp_adapter->GetDesc(&desc);

    return nowide::narrow(desc.Description);
}

DeviceDX::DeviceDX(const wrl::ComPtr<IDXGIAdapter>& cp_adapter, D3D_FEATURE_LEVEL feature_level)
    : DeviceBase(::GetAdapterName(cp_adapter), GetSupportedFeatures(cp_adapter, feature_level))
    , m_cp_adapter(cp_adapter)
    , m_feature_level(feature_level)
{
    ITT_FUNCTION_TASK();
}

DeviceDX::~DeviceDX()
{
    ITT_FUNCTION_TASK();
}

void DeviceDX::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();
    DeviceBase::SetName(name);
    if (m_cp_device)
    {
        m_cp_device->SetName(nowide::widen(name).c_str());
    }
}

const wrl::ComPtr<ID3D12Device>& DeviceDX::GetNativeDevice() const
{
    if (!m_cp_device)
    {
        ThrowIfFailed(D3D12CreateDevice(m_cp_adapter.Get(), m_feature_level, IID_PPV_ARGS(&m_cp_device)));
        if (!GetName().empty())
        {
            m_cp_device->SetName(nowide::widen(GetName()).c_str());
        }
    }
    return m_cp_device;
}

void DeviceDX::ReleaseNativeDevice()
{
    SafeRelease(m_cp_device);
}

System& System::Get()
{
    ITT_FUNCTION_TASK();
    static SystemDX s_system;
    return s_system;
}

SystemDX::SystemDX()
{
    ITT_FUNCTION_TASK();
    UINT dxgi_factory_flags = 0;

#ifdef _DEBUG
    // Enable the D3D12 debug layer.
    wrl::ComPtr<ID3D12Debug> cp_debug_controller;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&cp_debug_controller))))
    {
        cp_debug_controller->EnableDebugLayer();
        dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
    }
 #endif

    ThrowIfFailed(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&m_cp_factory)));
}

SystemDX::~SystemDX()
{
    ITT_FUNCTION_TASK();
}

const Devices& SystemDX::UpdateGpuDevices(Device::Feature::Mask supported_features)
{
    ITT_FUNCTION_TASK();
    assert(m_cp_factory);

    const D3D_FEATURE_LEVEL dx_feature_level = D3D_FEATURE_LEVEL_11_0;
    m_supported_features = supported_features;
    m_devices.clear();

    IDXGIAdapter1* p_adapter = nullptr;
    for (UINT adapter_index = 0; DXGI_ERROR_NOT_FOUND != m_cp_factory->EnumAdapters1(adapter_index, &p_adapter); ++adapter_index)
    {
        assert(p_adapter);
        if (!p_adapter)
            continue;

        DXGI_ADAPTER_DESC1 desc;
        p_adapter->GetDesc1(&desc);

        // Don't select the Basic Render Driver adapter.
        // If you want a software adapter, pass in "/warp" on the command line.
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        AddDevice(p_adapter, dx_feature_level);
    }

    wrl::ComPtr<IDXGIAdapter> cp_warp_adapter;
    m_cp_factory->EnumWarpAdapter(IID_PPV_ARGS(&cp_warp_adapter));
    if (cp_warp_adapter)
    {
        AddDevice(cp_warp_adapter, dx_feature_level);
    }

    return m_devices;
}

void SystemDX::AddDevice(const wrl::ComPtr<IDXGIAdapter>& cp_adapter, D3D_FEATURE_LEVEL feature_level)
{
    ITT_FUNCTION_TASK();

    // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet
    if (!SUCCEEDED(D3D12CreateDevice(cp_adapter.Get(), feature_level, _uuidof(ID3D12Device), nullptr)))
        return;

    Device::Feature::Mask device_supported_features = DeviceDX::GetSupportedFeatures(cp_adapter, feature_level);
    if (!(device_supported_features & m_supported_features))
        return;

    Device::Ptr sp_device = std::make_shared<DeviceDX>(cp_adapter, feature_level);
    m_devices.push_back(sp_device);
}
