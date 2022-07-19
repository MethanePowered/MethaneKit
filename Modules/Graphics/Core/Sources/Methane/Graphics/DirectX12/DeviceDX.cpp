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

FILE: Methane/Graphics/Metal/DeviceDX.cpp
DirectX 12 implementation of the device interface.

******************************************************************************/

#include "DeviceDX.h"

#include <Methane/Graphics/Windows/DirectXErrorHandling.h>
#include <Methane/Platform/Windows/Utils.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#ifdef _DEBUG
#include <dxgidebug.h>

// Uncomment to enable debugger breakpoint on DirectX debug warning or error
// #define BREAK_ON_DIRECTX_DEBUG_LAYER_MESSAGE_ENABLED
#endif

#include <magic_enum.hpp>
#include <nowide/convert.hpp>
#include <algorithm>
#include <cassert>

namespace Methane::Graphics
{

static std::string GetAdapterNameDxgi(IDXGIAdapter& adapter)
{
    META_FUNCTION_TASK();

    DXGI_ADAPTER_DESC desc{};
    adapter.GetDesc(&desc);
    return nowide::narrow(desc.Description);
}

static bool IsSoftwareAdapterDxgi(IDXGIAdapter1& adapter)
{
    META_FUNCTION_TASK();

    DXGI_ADAPTER_DESC1 desc{};
    adapter.GetDesc1(&desc);
    return desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;
}

#ifdef _DEBUG

static bool EnableDebugLayer()
{
    META_FUNCTION_TASK();
    wrl::ComPtr<ID3D12Debug> debug_controller_cptr;
    if (!SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller_cptr))))
    {
        META_LOG("WARNING: Unable to get D3D12 debug interface. " \
                 "Install 'Graphics Tools' in Windows optional features and try again.");
        return false;
    }

    debug_controller_cptr->EnableDebugLayer();

    wrl::ComPtr<IDXGIInfoQueue> dxgi_info_queue_cptr;
    if (!SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_info_queue_cptr))))
    {
        META_LOG("WARNING: Unable to get D3D12 info-queue interface.");
        return true;
    }

#ifdef BREAK_ON_DIRECTX_DEBUG_LAYER_MESSAGE_ENABLED
    dxgi_info_queue_cptr->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
    dxgi_info_queue_cptr->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
#endif

    std::array<DXGI_INFO_QUEUE_MESSAGE_ID, 0> skip_message_ids = {{ }};
    std::array<DXGI_INFO_QUEUE_MESSAGE_SEVERITY, 1> skip_message_severities = {{
        DXGI_INFO_QUEUE_MESSAGE_SEVERITY_INFO,
    }};

    DXGI_INFO_QUEUE_FILTER filter {};
    filter.DenyList.NumSeverities = static_cast<UINT>(skip_message_severities.size());
    filter.DenyList.pSeverityList = skip_message_severities.data();
    filter.DenyList.NumIDs        = static_cast<UINT>(skip_message_ids.size());
    filter.DenyList.pIDList       = skip_message_ids.data();
    dxgi_info_queue_cptr->AddStorageFilterEntries(DXGI_DEBUG_ALL, &filter);

    return true;
}

static void ConfigureDeviceDebugFeature(const wrl::ComPtr<ID3D12Device>& device_cptr)
{
    META_FUNCTION_TASK();
    wrl::ComPtr<ID3D12InfoQueue> device_info_queue_cptr;
    if (!SUCCEEDED(device_cptr->QueryInterface(IID_PPV_ARGS(&device_info_queue_cptr))))
        return;

#ifdef BREAK_ON_DIRECTX_DEBUG_LAYER_MESSAGE_ENABLED
    device_info_queue_cptr->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
    device_info_queue_cptr->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
#endif

    std::array<D3D12_MESSAGE_ID, 0> skip_message_ids = {{ }};
    std::array<D3D12_MESSAGE_SEVERITY, 1> skip_message_severities = {{
        D3D12_MESSAGE_SEVERITY_INFO,
    }};

    D3D12_INFO_QUEUE_FILTER filter {};
    filter.DenyList.NumSeverities = static_cast<UINT>(skip_message_severities.size());
    filter.DenyList.pSeverityList = skip_message_severities.data();
    filter.DenyList.NumIDs  = static_cast<UINT>(skip_message_ids.size());
    filter.DenyList.pIDList = skip_message_ids.data();
    device_info_queue_cptr->AddStorageFilterEntries(&filter);
}

#endif

Device::Features DeviceDX::GetSupportedFeatures(const wrl::ComPtr<IDXGIAdapter>& /*cp_adapter*/, D3D_FEATURE_LEVEL /*feature_level*/)
{
    META_FUNCTION_TASK();
    return Device::Features::BasicRendering;
}

DeviceDX::DeviceDX(const wrl::ComPtr<IDXGIAdapter>& cp_adapter, D3D_FEATURE_LEVEL feature_level, const Capabilities& capabilities)
    : DeviceBase(GetAdapterNameDxgi(*cp_adapter.Get()),
                 IsSoftwareAdapterDxgi(static_cast<IDXGIAdapter1&>(*cp_adapter.Get())),
                 capabilities)
    , m_cp_adapter(cp_adapter)
    , m_feature_level(feature_level)
{
    META_FUNCTION_TASK();
}

bool DeviceDX::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!DeviceBase::SetName(name))
        return false;

    if (m_cp_device)
    {
        m_cp_device->SetName(nowide::widen(name).c_str());
    }
    return true;
}

const wrl::ComPtr<ID3D12Device>& DeviceDX::GetNativeDevice() const
{
    META_FUNCTION_TASK();
    if (m_cp_device)
        return m_cp_device;

    ThrowIfFailed(D3D12CreateDevice(m_cp_adapter.Get(), m_feature_level, IID_PPV_ARGS(&m_cp_device)));
    if (!GetName().empty())
    {
        m_cp_device->SetName(nowide::widen(GetName()).c_str());
    }

    if (D3D12_FEATURE_DATA_D3D12_OPTIONS5 feature_options_5{};
        m_cp_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &feature_options_5, sizeof(feature_options_5)) == S_OK)
    {
        m_feature_options_5 = feature_options_5;
    }

#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
    if (Platform::Windows::IsDeveloperModeEnabled())
    {
        ThrowIfFailed(m_cp_device->SetStablePowerState(TRUE), m_cp_device.Get());
    }
    else
    {
        assert(false);
        META_LOG("WARNING: GPU instrumentation results may be unreliable because we failed to switch GPU to stable power state." \
                 "Enable Windows Developer Mode and try again.");
    }
#endif

#ifdef _DEBUG
    ConfigureDeviceDebugFeature(m_cp_device);
#endif

    return m_cp_device;
}

void DeviceDX::ReleaseNativeDevice()
{
    META_FUNCTION_TASK();
    m_cp_device.Reset();
}

System& System::Get()
{
    META_FUNCTION_TASK();
    static const auto s_system_ptr = std::make_shared<SystemDX>();
    return *s_system_ptr;
}

SystemDX::SystemDX()
{
    META_FUNCTION_TASK();
    Initialize();
}

SystemDX::~SystemDX()
{
    META_FUNCTION_TASK();

    UnregisterAdapterChangeEvent();

    m_cp_factory.Reset();

    ClearDevices();
    ReportLiveObjects();
}

void SystemDX::Initialize()
{
    META_FUNCTION_TASK();
    UINT dxgi_factory_flags = 0;

#ifdef _DEBUG
    if (EnableDebugLayer())
        dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&m_cp_factory)));
    META_CHECK_ARG_NOT_NULL(m_cp_factory);

    RegisterAdapterChangeEvent();
}

void SystemDX::RegisterAdapterChangeEvent()
{
    META_FUNCTION_TASK();

#ifdef ADAPTERS_CHANGE_HANDLING
    wrl::ComPtr<IDXGIFactory7> cp_factory7;
    if (!SUCCEEDED(m_cp_factory->QueryInterface(IID_PPV_ARGS(&cp_factory7))))
        return;

    m_adapter_change_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_adapter_change_event == nullptr)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    META_CHECK_ARG_NOT_NULL(cp_factory7);
    ThrowIfFailed(cp_factory7->RegisterAdaptersChangedEvent(m_adapter_change_event, &m_adapter_change_registration_cookie));
#endif
}

void SystemDX::UnregisterAdapterChangeEvent()
{
    META_FUNCTION_TASK();

#ifdef ADAPTERS_CHANGE_HANDLING
    wrl::ComPtr<IDXGIFactory7> cp_factory7;
    if (m_adapter_change_registration_cookie == 0 ||
        !SUCCEEDED(m_cp_factory->QueryInterface(IID_PPV_ARGS(&cp_factory7))))
        return;

    META_CHECK_ARG_NOT_NULL(cp_factory7);
    ThrowIfFailed(cp_factory7->UnregisterAdaptersChangedEvent(m_adapter_change_registration_cookie));
    m_adapter_change_registration_cookie = 0;

    CloseHandle(m_adapter_change_event);
    m_adapter_change_event = NULL;
#endif
}

void SystemDX::CheckForChanges()
{
    META_FUNCTION_TASK();

#ifdef ADAPTERS_CHANGE_HANDLING
    const bool adapters_changed = m_adapter_change_event ? WaitForSingleObject(m_adapter_change_event, 0) == WAIT_OBJECT_0
                                                         : !m_cp_factory->IsCurrent();

    if (!adapters_changed)
        return;

    UnregisterAdapterChangeEvent();
    Initialize();

    const Ptrs<Device>& devices = GetGpuDevices();
    Ptrs<Device>   prev_devices = devices;
    UpdateGpuDevices(GetDeviceCapabilities());

    for (const Ptr<Device>& prev_device_ptr : prev_devices)
    {
        META_CHECK_ARG_NOT_NULL(prev_device_ptr);
        DeviceDX& prev_device = static_cast<DeviceDX&>(*prev_device_ptr);
        auto device_it = std::find_if(devices.begin(), devices.end(),
                                      [prev_device](const Ptr<Device>& device_ptr)
                                      {
                                          DeviceDX& device = static_cast<DeviceDX&>(*device_ptr);
                                          return prev_device.GetNativeAdapter().GetAddressOf() == device.GetNativeAdapter().GetAddressOf();
                                      });

        if (device_it == devices.end())
        {
            RemoveDevice(prev_device);
        }
    }
#endif
}

const Ptrs<Device>& SystemDX::UpdateGpuDevices(const Platform::AppEnvironment&, const Device::Capabilities& required_device_caps)
{
    META_FUNCTION_TASK();
    return UpdateGpuDevices(required_device_caps);
}

const Ptrs<Device>& SystemDX::UpdateGpuDevices(const Device::Capabilities& required_device_caps)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_cp_factory);

    const D3D_FEATURE_LEVEL dx_feature_level = D3D_FEATURE_LEVEL_11_0;
    SetDeviceCapabilities(required_device_caps);
    ClearDevices();

    IDXGIAdapter1* p_adapter = nullptr;
    for (UINT adapter_index = 0; DXGI_ERROR_NOT_FOUND != m_cp_factory->EnumAdapters1(adapter_index, &p_adapter); ++adapter_index)
    {
        META_CHECK_ARG_NOT_NULL(p_adapter);
        if (IsSoftwareAdapterDxgi(*p_adapter))
            continue;

        AddDevice(p_adapter, dx_feature_level);
    }

    wrl::ComPtr<IDXGIAdapter> cp_warp_adapter;
    m_cp_factory->EnumWarpAdapter(IID_PPV_ARGS(&cp_warp_adapter));
    if (cp_warp_adapter)
    {
        AddDevice(cp_warp_adapter, dx_feature_level);
    }

    return GetGpuDevices();
}

void SystemDX::AddDevice(const wrl::ComPtr<IDXGIAdapter>& cp_adapter, D3D_FEATURE_LEVEL feature_level)
{
    META_FUNCTION_TASK();

    // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet
    if (!SUCCEEDED(D3D12CreateDevice(cp_adapter.Get(), feature_level, _uuidof(ID3D12Device), nullptr)))
        return;

    Device::Features device_supported_features = DeviceDX::GetSupportedFeatures(cp_adapter, feature_level);

    using namespace magic_enum::bitwise_operators;
    if (!static_cast<bool>(device_supported_features & GetDeviceCapabilities().features))
        return;

    SystemBase::AddDevice(std::make_shared<DeviceDX>(cp_adapter, feature_level, GetDeviceCapabilities()));
}

void SystemDX::ReportLiveObjects() const noexcept
{
    META_FUNCTION_TASK();
#ifdef _DEBUG
    wrl::ComPtr<IDXGIDebug1> dxgi_debug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug))) && dxgi_debug)
    {
        dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
    }
#endif
}

} // namespace Methane::Graphics
