/******************************************************************************

Copyright 2019-2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/DirectX/IResource.h
DirectX 12 specialization of the resource interface.

******************************************************************************/

#pragma once

#include "IContext.h"
#include "DescriptorHeap.h"
#include "ResourceBarriers.h"
#include "ResourceView.h"

#include <Methane/Graphics/RHI/IResource.h>

#include <wrl.h>
#include <directx/d3d12.h>

namespace Methane::Graphics::DirectX
{

namespace wrl = Microsoft::WRL;

struct IResource
    : virtual Rhi::IResource // NOSONAR
{
public:
    using Barrier   = Rhi::ResourceBarrier;
    using Barriers  = ResourceBarriers;
    using State     = Rhi::ResourceState;
    using View      = ResourceView;
    using Views     = ResourceViews;

    [[nodiscard]] static DescriptorHeap::Type  GetDescriptorHeapTypeByUsage(const Rhi::IResource& resource, UsageMask resource_usage);
    [[nodiscard]] static D3D12_RESOURCE_STATES GetNativeResourceState(State resource_state);

    [[nodiscard]] virtual ID3D12Resource&                    GetNativeResourceRef() const = 0;
    [[nodiscard]] virtual ID3D12Resource*                    GetNativeResource() const noexcept = 0;
    [[nodiscard]] virtual const wrl::ComPtr<ID3D12Resource>& GetNativeResourceComPtr() const noexcept = 0;
    [[nodiscard]] virtual D3D12_GPU_VIRTUAL_ADDRESS          GetNativeGpuAddress() const noexcept = 0;

    virtual Opt<Descriptor> InitializeNativeViewDescriptor(const View::Id& view_id) = 0;

    ~IResource() override = default;
};

} // namespace Methane::Graphics::DirectX
