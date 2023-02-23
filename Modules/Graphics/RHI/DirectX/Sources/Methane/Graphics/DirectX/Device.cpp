
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

FILE: Methane/Graphics/Metal/Device.cpp
DirectX 12 implementation of the device interface.

******************************************************************************/

#include <Methane/Graphics/DirectX/Device.h>
#include <Methane/Graphics/DirectX/RenderContext.h>
#include <Methane/Graphics/DirectX/ComputeContext.h>
#include <Methane/Graphics/DirectX/ErrorHandling.h>

#include <Methane/Platform/Windows/Utils.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#ifdef _DEBUG
#include <dxgidebug.h>

// Uncomment to enable debugger breakpoint on DirectX debug warning or error
// #define BREAK_ON_DIRECTX_DEBUG_LAYER_MESSAGE_ENABLED
#endif

#include <nowide/convert.hpp>
#include <array>
#include <algorithm>
#include <cassert>

namespace Methane::Graphics::DirectX
{

static std::string GetAdapterNameDxgi(IDXGIAdapter& adapter)
{
    META_FUNCTION_TASK();
    DXGI_ADAPTER_DESC desc{};
    adapter.GetDesc(&desc);
    return nowide::narrow(desc.Description);
}

bool IsSoftwareAdapterDxgi(IDXGIAdapter1& adapter)
{
    META_FUNCTION_TASK();
    DXGI_ADAPTER_DESC1 desc{};
    adapter.GetDesc1(&desc);
    return desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;
}

#ifdef _DEBUG

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

Rhi::DeviceFeatureMask Device::GetSupportedFeatures(const wrl::ComPtr<IDXGIAdapter>& /*cp_adapter*/, D3D_FEATURE_LEVEL /*feature_level*/)
{
    META_FUNCTION_TASK();
    Rhi::DeviceFeatureMask supported_features;
    // TODO: implement adapter features detection for DirectX
    supported_features.SetBitOn(Rhi::DeviceFeature::PresentToWindow);
    supported_features.SetBitOn(Rhi::DeviceFeature::AnisotropicFiltering);
    supported_features.SetBitOn(Rhi::DeviceFeature::ImageCubeArray);
    return supported_features;
}

Device::Device(const wrl::ComPtr<IDXGIAdapter>& cp_adapter, D3D_FEATURE_LEVEL feature_level, const Capabilities& capabilities)
    : Base::Device(GetAdapterNameDxgi(*cp_adapter.Get()),
                   IsSoftwareAdapterDxgi(static_cast<IDXGIAdapter1&>(*cp_adapter.Get())),
                   capabilities)
    , m_cp_adapter(cp_adapter)
    , m_feature_level(feature_level)
{ }

Ptr<Rhi::IRenderContext> Device::CreateRenderContext(const Platform::AppEnvironment& env, tf::Executor& parallel_executor, const Rhi::RenderContextSettings& settings)
{
    META_FUNCTION_TASK();
    auto render_context_ptr = std::make_shared<RenderContext>(env, *this, parallel_executor, settings);
    render_context_ptr->Initialize(*this, true);
    return render_context_ptr;
}

Ptr<Rhi::IComputeContext> Device::CreateComputeContext(tf::Executor& parallel_executor, const Rhi::ComputeContextSettings& settings)
{
    META_FUNCTION_TASK();
    const auto compute_context_ptr = std::make_shared<Vulkan::ComputeContext>(*this, parallel_executor, settings);
    compute_context_ptr->Initialize(*this, true);
    return compute_context_ptr;
}

bool Device::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Base::Device::SetName(name))
        return false;

    if (m_cp_device)
    {
        m_cp_device->SetName(nowide::widen(name).c_str());
    }
    return true;
}

const wrl::ComPtr<ID3D12Device>& Device::GetNativeDevice() const
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

void Device::ReleaseNativeDevice()
{
    META_FUNCTION_TASK();
    m_cp_device.Reset();
}

} // namespace Methane::Graphics::DirectX
