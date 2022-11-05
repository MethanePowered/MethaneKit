/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/DirectX12/ResourceDX.cpp
DirectX 12 specialization of the resource interface.

******************************************************************************/

#include <Methane/Graphics/ResourceDX.h>
#include <Methane/Graphics/BufferDX.h>
#include <Methane/Graphics/TextureDX.h>
#include <Methane/Graphics/SamplerDX.h>
#include <Methane/Graphics/ContextDX.h>
#include <Methane/Graphics/DescriptorManagerDX.h>

#include <Methane/Graphics/IContext.h>
#include <Methane/Graphics/ITexture.h>
#include <Methane/Graphics/Base/Resource.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

DescriptorHeapDX::Type IResourceDX::GetDescriptorHeapTypeByUsage(const IResource& resource, IResource::Usage resource_usage)
{
    META_FUNCTION_TASK();
    const IResource::Type resource_type = resource.GetResourceType();
    switch (resource_usage)
    {
    case IResource::Usage::ShaderRead:
        return (resource_type == IResource::Type::Sampler)
               ? DescriptorHeapDX::Type::Samplers
               : DescriptorHeapDX::Type::ShaderResources;

    case IResource::Usage::ShaderWrite:
    case IResource::Usage::RenderTarget:
        return (resource_type == IResource::Type::Texture &&
                dynamic_cast<const ITexture&>(resource).GetSettings().type == ITexture::Type::DepthStencilBuffer)
               ? DescriptorHeapDX::Type::DepthStencil
               : DescriptorHeapDX::Type::RenderTargets;

    default:
        META_UNEXPECTED_ARG_DESCR_RETURN(resource_usage, DescriptorHeapDX::Type::Undefined,
                                         "resource usage does not map to descriptor heap");
    }
}

ResourceViewDX::ResourceViewDX(const ResourceView& view_id, IResource::Usage usage)
    : ResourceView(view_id)
    , m_id(usage, GetSettings())
    , m_resource_dx(dynamic_cast<IResourceDX&>(GetResource()))
    , m_descriptor_opt(m_resource_dx.InitializeNativeViewDescriptor(m_id))
{
    META_FUNCTION_TASK();
}

D3D12_GPU_VIRTUAL_ADDRESS ResourceViewDX::GetNativeGpuAddress() const noexcept
{
    META_FUNCTION_TASK();
    return m_resource_dx.GetNativeGpuAddress() + GetOffset();
}

D3D12_CPU_DESCRIPTOR_HANDLE ResourceViewDX::GetNativeCpuDescriptorHandle() const noexcept
{
    META_FUNCTION_TASK();
    return m_descriptor_opt ? m_descriptor_opt->heap.GetNativeCpuDescriptorHandle(m_descriptor_opt->index)
                            : D3D12_CPU_DESCRIPTOR_HANDLE{ 0U };
}

D3D12_GPU_DESCRIPTOR_HANDLE ResourceViewDX::GetNativeGpuDescriptorHandle() const noexcept
{
    META_FUNCTION_TASK();
    return m_descriptor_opt ? m_descriptor_opt->heap.GetNativeGpuDescriptorHandle(m_descriptor_opt->index)
                            : D3D12_GPU_DESCRIPTOR_HANDLE{ 0U };
}

} // namespace Methane::Graphics