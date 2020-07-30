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
#pragma once

#include <Methane/Memory.hpp>
#include <Methane/Instrumentation.h>

#include <mutex>

namespace Methane::Graphics
{

class ContextBase;
class ResourceBase;

struct ReleasePool
{
    struct RetainedResource
    {
        virtual ~RetainedResource() = default;
    };

    ReleasePool(ContextBase& context);

    void AddResource(UniquePtr<RetainedResource>&& retained_resource);
    void AddUploadResource(UniquePtr<RetainedResource>&& retained_resource);
    void ReleaseFrameResources(uint32_t frame_index);
    void ReleaseUploadResources();
    void ReleaseAllResources();

private:
    using RetainedResources = UniquePtrs<RetainedResource>;

    ContextBase&                   m_context;
    std::vector<RetainedResources> m_frame_resources;
    RetainedResources              m_upload_resources;
    RetainedResources              m_misc_resources;
    TracyLockable(std::mutex,      m_mutex);
};

}