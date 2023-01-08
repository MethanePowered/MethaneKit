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

FILE: Methane/Graphics/DirectX/Device.h
DirectX 12 implementation of the device interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/Device.h>

#include <wrl.h>
#include <dxgi1_6.h>
#include <directx/d3d12.h>

#include <optional>

// NOTE: Adapters change handling breaks many frame capture tools, like VS or RenderDoc
//#define ADAPTERS_CHANGE_HANDLING

namespace Methane::Graphics::DirectX
{

namespace wrl = Microsoft::WRL;

class Device final : public Base::Device
{
public:
    static Rhi::DeviceFeatureMask GetSupportedFeatures(const wrl::ComPtr<IDXGIAdapter>& cp_adapter, D3D_FEATURE_LEVEL feature_level);

    Device(const wrl::ComPtr<IDXGIAdapter>& cp_adapter, D3D_FEATURE_LEVEL feature_level, const Capabilities& capabilities);

    // IDevice interface
    [[nodiscard]] Ptr<Rhi::IRenderContext> CreateRenderContext(const Platform::AppEnvironment& env, tf::Executor& parallel_executor, const Rhi::RenderContextSettings& settings) override;

    // IObject interface
    bool SetName(std::string_view name) override;

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

bool IsSoftwareAdapterDxgi(IDXGIAdapter1& adapter);

} // namespace Methane::Graphics::DirectX
