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

FILE: Methane/Graphics/ReleasePool.h
GPU release pool for deferred objects release when they are not used by GPU anymore.

******************************************************************************/

#include "ReleasePool.h"
#include "ResourceBase.h"
#include "RenderContextBase.h"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

ReleasePool::ReleasePool(ContextBase& context)
    : m_context(context)
{
    META_FUNCTION_TASK();
}

void ReleasePool::AddResource(UniquePtr<RetainedResource>&& retained_resource)
{
    META_FUNCTION_TASK();
    if (!retained_resource)
        return;

    if (m_context.GetType() == Context::Type::Render)
    {
        RenderContextBase& render_context = static_cast<RenderContextBase&>(m_context);
        if (m_frame_resources.size() != render_context.GetSettings().frame_buffers_count)
            m_frame_resources.resize(render_context.GetSettings().frame_buffers_count);

        const uint32_t frame_index = render_context.GetFrameBufferIndex();
        m_frame_resources[frame_index].emplace_back(std::move(retained_resource));
    }
    else
    {
        m_misc_resources.emplace_back(std::move(retained_resource));
    }
}

void ReleasePool::ReleaseAllResources()
{
    META_FUNCTION_TASK();
    for(RetainedResources& frame_resources : m_frame_resources)
    {
        frame_resources.clear();
    }
    m_misc_resources.clear();
}

void ReleasePool::ReleaseFrameResources(uint32_t frame_index)
{
    META_FUNCTION_TASK();
    if (frame_index >= m_frame_resources.size())
        return;

    m_frame_resources[frame_index].clear();
}

} // namespace Methane::Graphics