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

#include "ResourceDX.h"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

ResourceLocationDX::ResourceLocationDX(const ResourceLocation& location, Resource::Usage usage)
    : ResourceLocation(location)
    , m_usage(usage)
    , m_resource_dx(dynamic_cast<IResourceDX&>(GetResource()))
{
    META_FUNCTION_TASK();
}

[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS ResourceLocationDX::GetNativeGpuAddress() const noexcept
{
    return m_resource_dx.get().GetNativeGpuAddress() + GetOffset();
}

D3D12_CPU_DESCRIPTOR_HANDLE ResourceLocationDX::GetNativeCpuDescriptorHandle() const noexcept
{
    META_FUNCTION_TASK();
    return m_resource_dx.get().GetNativeCpuDescriptorHandle(m_usage);
}

D3D12_GPU_DESCRIPTOR_HANDLE ResourceLocationDX::GetNativeGpuDescriptorHandle() const noexcept
{
    META_FUNCTION_TASK();
    return m_resource_dx.get().GetNativeGpuDescriptorHandle(m_usage);
}

} // namespace Methane::Graphics