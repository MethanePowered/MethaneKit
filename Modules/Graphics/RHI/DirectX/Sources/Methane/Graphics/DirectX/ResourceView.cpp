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

FILE: Methane/Graphics/DirectX/Resource.cpp
DirectX 12 specialization of the resource interface.

******************************************************************************/

#include <Methane/Graphics/DirectX/IResource.h>
#include <Methane/Graphics/DirectX/Buffer.h>
#include <Methane/Graphics/DirectX/Texture.h>
#include <Methane/Graphics/DirectX/Sampler.h>
#include <Methane/Graphics/DirectX/IContext.h>
#include <Methane/Graphics/DirectX/DescriptorManager.h>

#include <Methane/Graphics/RHI/IContext.h>
#include <Methane/Graphics/RHI/ITexture.h>
#include <Methane/Graphics/Base/Resource.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::DirectX
{

ResourceDescriptor::ResourceDescriptor(DescriptorHeap& in_heap, Data::Index in_index)
    : heap(in_heap)
    , index(in_index)
{ }

ResourceView::ResourceView(const Rhi::ResourceView& view_id, Rhi::ResourceUsageMask usage)
    : Rhi::ResourceView(view_id)
    , m_id(usage, GetSettings())
    , m_resource_dx(dynamic_cast<IResource&>(GetResource()))
    , m_descriptor_opt(m_resource_dx.InitializeNativeViewDescriptor(m_id))
{ }

D3D12_GPU_VIRTUAL_ADDRESS ResourceView::GetNativeGpuAddress() const noexcept
{
    META_FUNCTION_TASK();
    return m_resource_dx.GetNativeGpuAddress() + GetOffset();
}

D3D12_CPU_DESCRIPTOR_HANDLE ResourceView::GetNativeCpuDescriptorHandle() const noexcept
{
    META_FUNCTION_TASK();
    return m_descriptor_opt ? m_descriptor_opt->heap.GetNativeCpuDescriptorHandle(m_descriptor_opt->index)
                            : D3D12_CPU_DESCRIPTOR_HANDLE{ 0U };
}

D3D12_GPU_DESCRIPTOR_HANDLE ResourceView::GetNativeGpuDescriptorHandle() const noexcept
{
    META_FUNCTION_TASK();
    return m_descriptor_opt ? m_descriptor_opt->heap.GetNativeGpuDescriptorHandle(m_descriptor_opt->index)
                            : D3D12_GPU_DESCRIPTOR_HANDLE{ 0U };
}

} // namespace Methane::Graphics