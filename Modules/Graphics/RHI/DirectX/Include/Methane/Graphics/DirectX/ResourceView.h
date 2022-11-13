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

FILE: Methane/Graphics/DirectX/ResourceView.h
DirectX 12 specialization of the resource interface.

******************************************************************************/

#pragma once

#include "IContextDx.h"
#include "DescriptorHeap.h"
#include "ResourceBarriers.h"

#include <Methane/Graphics/RHI/IResource.h>

#include <wrl.h>
#include <directx/d3d12.h>

namespace Methane::Graphics::DirectX
{

namespace wrl = Microsoft::WRL;

struct IResourceDx;

class ResourceView final
    : public Rhi::ResourceView
{
public:
    ResourceView(const Rhi::ResourceView& view_id, Rhi::ResourceUsage usage);

    [[nodiscard]] const Id& GetId() const noexcept
    { return m_id; }

    [[nodiscard]] Rhi::ResourceUsage GetUsage() const noexcept
    { return m_id.usage; }

    [[nodiscard]] IResourceDx& GetDirectResource() const noexcept
    { return m_resource_dx; }

    [[nodiscard]] bool HasDescriptor() const noexcept
    { return m_descriptor_opt.has_value(); }

    [[nodiscard]] const Opt<ResourceDescriptor>& GetDescriptor() const noexcept
    { return m_descriptor_opt; }

    [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS   GetNativeGpuAddress() const noexcept;
    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetNativeCpuDescriptorHandle() const noexcept;
    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetNativeGpuDescriptorHandle() const noexcept;

private:
    Id                      m_id;
    IResourceDx&            m_resource_dx;
    Opt<ResourceDescriptor> m_descriptor_opt;
};

using ResourceViews = std::vector<ResourceView>;

} // namespace Methane::Graphics::DirectX
