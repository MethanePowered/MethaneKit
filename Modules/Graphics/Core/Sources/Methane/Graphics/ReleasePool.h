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

namespace Methane::Graphics
{

class ResourceBase;

struct ReleasePool
{
    static Ptr<ReleasePool> Create();

    virtual void AddResource(ResourceBase& resource) = 0;
    virtual void ReleaseAllResources() = 0;
    virtual void ReleaseFrameResources(uint32_t frame_index) = 0;

    virtual ~ReleasePool() = default;
};

}