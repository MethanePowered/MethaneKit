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

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include <nowide/convert.hpp>
#include <algorithm>
#include <cassert>

namespace Methane
{
namespace Graphics
{

Device::Feature::Mask DeviceDX::GetSupportedFeatures(const wrl::ComPtr<IDXGIAdapter>& cp_adapter, D3D_FEATURE_LEVEL feature_level)
{
    ITT_FUNCTION_TASK();
    Device::Feature::Mask supported_featues = Device::Feature::Value::BasicRendering;
    cp_adapter;
    feature_level;
    return supported_featues;
}

std::string GetAdapterNameDXGI(IDXGIAdapter& adapter)
{
    ITT_FUNCTION_TASK();

    DXGI_ADAPTER_DESC desc = {};
    adapter.GetDesc(&desc);
    return nowide::narrow(desc.Description);
}

bool IsSoftwareAdapterDXGI(IDXGIAdapter1& adapter)
{
    DXGI_ADAPTER_DESC1 desc = {};
    adapter.GetDesc1(&desc);
    return desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;
}

DeviceDX::DeviceDX(const wrl::ComPtr<IDXGIAdapter>& cp_adapter, D3D_FEATURE_LEVEL feature_level)
    : DeviceBase(GetAdapterNameDXGI(*cp_adapter.Get()),
                 IsSoftwareAdapterDXGI(static_cast<IDXGIAdapter1&>(*cp_adapter.Get())),
                 GetSupportedFeatures(cp_adapter, feature_level))
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
    m_cp_device.Reset();
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
    Initialize();
}

SystemDX::~SystemDX()
{
    ITT_FUNCTION_TASK();

    UnregisterAdapterChangeEvent();

    m_cp_factory.Reset();
    m_devices.clear();

    ReportLiveObjects();
}

void SystemDX::Initialize()
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
    assert(!!m_cp_factory);

    RegisterAdapterChangeEvent();
}

void SystemDX::RegisterAdapterChangeEvent()
{
    ITT_FUNCTION_TASK();

    wrl::ComPtr<IDXGIFactory7> cp_factory7;
    if (!SUCCEEDED(m_cp_factory->QueryInterface(IID_PPV_ARGS(&cp_factory7))))
        return;

    m_adapter_change_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_adapter_change_event == nullptr)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    ThrowIfFailed(cp_factory7->RegisterAdaptersChangedEvent(m_adapter_change_event, &m_adapter_change_registration_cookie));
}

void SystemDX::UnregisterAdapterChangeEvent()
{
    ITT_FUNCTION_TASK();

    wrl::ComPtr<IDXGIFactory7> cp_factory7;
    if (m_adapter_change_registration_cookie == 0 ||
        !SUCCEEDED(m_cp_factory->QueryInterface(IID_PPV_ARGS(&cp_factory7))))
        return;

    ThrowIfFailed(cp_factory7->UnregisterAdaptersChangedEvent(m_adapter_change_registration_cookie));
    m_adapter_change_registration_cookie = 0;

    CloseHandle(m_adapter_change_event);
    m_adapter_change_event = NULL;
}

void SystemDX::CheckForChanges()
{
    ITT_FUNCTION_TASK();
    const bool adapters_changed = m_adapter_change_event ? WaitForSingleObject(m_adapter_change_event, 0) == WAIT_OBJECT_0
                                                         : !m_cp_factory->IsCurrent();

    if (!adapters_changed)
        return;

    UnregisterAdapterChangeEvent();
    Initialize();

    Devices prev_devices = m_devices;
    UpdateGpuDevices(m_supported_features);

    for (const Device::Ptr& sp_prev_device : prev_devices)
    {
        assert(!!sp_prev_device);
        DeviceDX& prev_device = static_cast<DeviceDX&>(*sp_prev_device);
        auto device_it = std::find_if(m_devices.begin(), m_devices.end(),
                                      [prev_device](const Device::Ptr& sp_device)
                                      {
                                          DeviceDX& device = static_cast<DeviceDX&>(*sp_device);
                                          return prev_device.GetNativeAdapter().GetAddressOf() == device.GetNativeAdapter().GetAddressOf();
                                      });

        if (device_it == m_devices.end())
        {
            prev_device.Notify(Device::Notification::Removed);
        }
    }
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

        // Don't select the Basic Render Driver adapter.
        // If you want a software adapter, pass in "/warp" on the command line.
        if (IsSoftwareAdapterDXGI(*p_adapter))
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

void SystemDX::ReportLiveObjects()
{
    ITT_FUNCTION_TASK();
#ifdef _DEBUG
    wrl::ComPtr<IDXGIDebug1> dxgi_debug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug))))
    {
        assert(!!dxgi_debug);
        dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
    }
#endif
}

} // namespace Graphics
} // namespace Methane
