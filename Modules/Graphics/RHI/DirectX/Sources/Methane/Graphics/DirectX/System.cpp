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

FILE: Methane/Graphics/Metal/System.cpp
DirectX 12 implementation of the device interface.

******************************************************************************/

#include <Methane/Graphics/DirectX/System.h>
#include <Methane/Graphics/DirectX/Device.h>
#include <Methane/Graphics/DirectX/ErrorHandling.h>

#include <Methane/Platform/Windows/Utils.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#ifdef _DEBUG
#include <dxgidebug.h>

// Uncomment to enable debugger breakpoint on DirectX debug warning or error
#define BREAK_ON_DIRECTX_DEBUG_LAYER_MESSAGE_ENABLED
#endif

#include <nowide/convert.hpp>
#include <array>
#include <algorithm>
#include <cassert>

namespace Methane::Graphics::Rhi
{

Rhi::ISystem& Rhi::ISystem::Get()
{
    META_FUNCTION_TASK();
    static const auto s_system_ptr = std::make_shared<DirectX::System>();
    return *s_system_ptr;
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::DirectX
{

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
    ThrowIfFailed(dxgi_info_queue_cptr->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, true));
    ThrowIfFailed(dxgi_info_queue_cptr->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true));
    ThrowIfFailed(dxgi_info_queue_cptr->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true));
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
    ThrowIfFailed(dxgi_info_queue_cptr->AddStorageFilterEntries(DXGI_DEBUG_ALL, &filter));

    return true;
}

#endif

System::System()
{
    META_FUNCTION_TASK();
    Initialize();
}

System::~System()
{
    META_FUNCTION_TASK();

#ifdef ADAPTERS_CHANGE_HANDLING
    UnregisterAdapterChangeEvent();
#endif

    m_factory_cptr.Reset();

    ClearDevices();
    ReportLiveObjects();
}

void System::Initialize()
{
    META_FUNCTION_TASK();
    UINT dxgi_factory_flags = 0;

#ifdef _DEBUG
    if (EnableDebugLayer())
        dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&m_factory_cptr)));
    META_CHECK_NOT_NULL(m_factory_cptr);

#ifdef ADAPTERS_CHANGE_HANDLING
    RegisterAdapterChangeEvent();
#endif
}

#ifdef ADAPTERS_CHANGE_HANDLING

void System::RegisterAdapterChangeEvent()
{
    META_FUNCTION_TASK();
    wrl::ComPtr<IDXGIFactory7 factory7_cptr;
    if (!SUCCEEDED(m_factory_cptr->QueryInterface(IID_PPV_ARGS&factory7_cptr))))
        return;

    m_adapter_change_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_adapter_change_event == nullptr)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    META_CHECK_NOT_NULL(factory7_cptr);
    ThrowIfFailed(factory7_cptr->RegisterAdaptersChangedEvent(m_adapter_change_event, &m_adapter_change_registration_cookie));
}

void System::UnregisterAdapterChangeEvent()
{
    META_FUNCTION_TASK();
    wrl::ComPtr<IDXGIFactory7 factory7_cptr;
    if (m_adapter_change_registration_cookie == 0 ||
        !SUCCEEDED(m_factory_cptr->QueryInterface(IID_PPV_ARGS&factory7_cptr))))
        return;

    META_CHECK_NOT_NULL(factory7_cptr);
    ThrowIfFailed(factory7_cptr->UnregisterAdaptersChangedEvent(m_adapter_change_registration_cookie));
    m_adapter_change_registration_cookie = 0;

    CloseHandle(m_adapter_change_event);
    m_adapter_change_event = NULL;
}

#endif // def ADAPTERS_CHANGE_HANDLING

void System::CheckForChanges()
{
    META_FUNCTION_TASK();

#ifdef ADAPTERS_CHANGE_HANDLING
    const bool adapters_changed = m_adapter_change_event ? WaitForSingleObject(m_adapter_change_event, 0) == WAIT_OBJECT_0
                                                         : !m_factory_cptr->IsCurrent();

    if (!adapters_changed)
        return;

#ifdef ADAPTERS_CHANGE_HANDLING
    UnregisterAdapterChangeEvent();
#endif

    Initialize();

    const Ptrs<IDevice>& devices = GetGpuDevices();
    Ptrs<IDevice>   prev_devices = devices;
    UpdateGpuDevices(GetDeviceCapabilities());

    for (const Ptr<IDevice>& prev_device_ptr : prev_devices)
    {
        META_CHECK_NOT_NULL(prev_device_ptr);
        Device& prev_device = static_cast<Device&>(*prev_device_ptr);
        auto device_it = std::find_if(devices.begin(), devices.end(),
                                      [prev_device](const Ptr<IDevice>& device_ptr)
                                      {
                                          Device& device = static_cast<Device&>(*device_ptr);
                                          return prev_device.GetNativeAdapter().GetAddressOf() == device.GetNativeAdapter().GetAddressOf();
                                      });

        if (device_it == devices.end())
        {
            RemoveDevice(prev_device);
        }
    }
#endif
}

const Ptrs<Rhi::IDevice>& System::UpdateGpuDevices(const Platform::AppEnvironment&, const Rhi::DeviceCaps& required_device_caps)
{
    META_FUNCTION_TASK();
    return UpdateGpuDevices(required_device_caps);
}

const Ptrs<Rhi::IDevice>& System::UpdateGpuDevices(const Rhi::DeviceCaps& required_device_caps)
{
    META_FUNCTION_TASK();
    META_CHECK_NOT_NULL(m_factory_cptr);

    const D3D_FEATURE_LEVEL dx_feature_level = D3D_FEATURE_LEVEL_11_0;
    SetDeviceCapabilities(required_device_caps);
    ClearDevices();

    IDXGIAdapter1* adapter_ptr = nullptr;
    for (UINT adapter_index = 0; DXGI_ERROR_NOT_FOUND != m_factory_cptr->EnumAdapters1(adapter_index, &adapter_ptr); ++adapter_index)
    {
        META_CHECK_NOT_NULL(adapter_ptr);
        if (IsSoftwareAdapterDxgi(*adapter_ptr))
            continue;

        AddDevice(adapter_ptr, dx_feature_level);
    }

    wrl::ComPtr<IDXGIAdapter> warp_adapter_cptr;
    m_factory_cptr->EnumWarpAdapter(IID_PPV_ARGS(&warp_adapter_cptr));
    if(warp_adapter_cptr)
    {
        AddDevice(warp_adapter_cptr, dx_feature_level);
    }

    return GetGpuDevices();
}

void System::AddDevice(const wrl::ComPtr<IDXGIAdapter>& adapter_cptr, D3D_FEATURE_LEVEL feature_level)
{
    META_FUNCTION_TASK();

    // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet
    if (!SUCCEEDED(D3D12CreateDevice(adapter_cptr.Get(), feature_level, _uuidof(ID3D12Device), nullptr)))
        return;

    if (const Rhi::DeviceFeatureMask device_supported_features = Device::GetSupportedFeatures(adapter_cptr, feature_level);
        !device_supported_features.HasBits(GetDeviceCapabilities().features))
        return;

    Base::System::AddDevice(std::make_shared<Device>(adapter_cptr, feature_level, GetDeviceCapabilities()));
}

void System::ReportLiveObjects() const noexcept
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

} // namespace Methane::Graphics::DirectX
