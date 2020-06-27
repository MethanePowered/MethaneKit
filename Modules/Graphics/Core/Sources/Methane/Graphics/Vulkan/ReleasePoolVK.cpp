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

FILE: Methane/Graphics/Vulkan/ReleasePoolVK.cpp
Vulkan GPU release pool for deferred objects release.

******************************************************************************/

#include "ReleasePoolVK.h"

#include <Methane/Graphics/RenderContextBase.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

struct ResourceContainerVK
{
};

Ptr<ReleasePool> ReleasePool::Create()
{
    META_FUNCTION_TASK();
    return std::make_shared<ReleasePoolVK>();
}

ReleasePoolVK::ReleasePoolVK()
    : ReleasePool()
    , m_sp_vk_resources(new ResourceContainerVK())
{
    META_FUNCTION_TASK();
}

void ReleasePoolVK::AddResource(ResourceBase& /*resource*/)
{
    META_FUNCTION_TASK();
}

void ReleasePoolVK::ReleaseAllResources()
{
    META_FUNCTION_TASK();
    m_sp_vk_resources.reset(new ResourceContainerVK());
}

void ReleasePoolVK::ReleaseFrameResources(uint32_t frame_index)
{
    META_FUNCTION_TASK();
    META_UNUSED(frame_index);

    // TODO: to be implemented
    ReleaseAllResources();
}

} // namespace Methane::Graphics