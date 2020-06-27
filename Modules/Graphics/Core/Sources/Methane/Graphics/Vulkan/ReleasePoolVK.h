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

FILE: Methane/Graphics/Vulkan/ReleasePoolVK.h
Vulkan GPU release pool for deferred objects release.

******************************************************************************/
#pragma once

#include <Methane/Graphics/ReleasePool.h>

#include <vector>

namespace Methane::Graphics
{

struct ResourceContainerVK;

class ReleasePoolVK final : public ReleasePool
{
public:
    ReleasePoolVK();

    // ReleasePool interface
    void AddResource(ResourceBase& resource) override;
    void ReleaseAllResources() override;
    void ReleaseFrameResources(uint32_t frame_index) override;

private:
    std::unique_ptr<ResourceContainerVK> m_sp_vk_resources;
};

}