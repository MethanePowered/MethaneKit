/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/DirectX12/ReleasePoolDX.h
DirectX 12 GPU release pool for deferred objects release.

******************************************************************************/

#include "ReleasePoolDX.h"
#include "ResourceDX.h"

#include <Methane/Graphics/RenderContextBase.h>
#include <Methane/Graphics/Windows/Primitives.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<ReleasePool> ReleasePool::Create()
{
    META_FUNCTION_TASK();
    return std::make_shared<ReleasePoolDX>();
}

void ReleasePoolDX::AddResource(ResourceBase& resource)
{
    META_FUNCTION_TASK();
    ResourceDX& resource_dx = static_cast<ResourceDX&>(resource);

    const wrl::ComPtr<ID3D12Resource>& cp_native_resource = resource_dx.GetNativeResourceComPtr();
    assert(cp_native_resource || resource_dx.GetResourceType() == Resource::Type::Sampler);
    if (!cp_native_resource)
        return;

    if (resource_dx.GetContextBase().GetType() == Context::Type::Render)
    {
        RenderContextBase& render_context = static_cast<RenderContextBase&>(resource_dx.GetContextBase());
        if (m_frame_resources.size() != render_context.GetSettings().frame_buffers_count)
            m_frame_resources.resize(render_context.GetSettings().frame_buffers_count);

        const uint32_t frame_index = render_context.GetFrameBufferIndex();
        m_frame_resources[frame_index].emplace_back(cp_native_resource);
    }
    else
    {
        m_misc_resources.emplace_back(cp_native_resource);
    }
}

void ReleasePoolDX::ReleaseAllResources()
{
    META_FUNCTION_TASK();
    for (D3DResourceComPtrs& frame_resources : m_frame_resources)
    {
        frame_resources.clear();
    }
    m_misc_resources.clear();
}

void ReleasePoolDX::ReleaseFrameResources(uint32_t frame_index)
{
    META_FUNCTION_TASK();
    if (frame_index >= m_frame_resources.size())
        return;

    m_frame_resources[frame_index].clear();
}

} // namespace Methane::Graphics