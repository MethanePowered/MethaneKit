/******************************************************************************

Copyright 2020-2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/ResourceVK.cpp
Vulkan implementation of the resource interface.

******************************************************************************/

#include "ResourceBarriersVK.h"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<ResourceBarriers> ResourceBarriers::Create(const Set& barriers)
{
    META_FUNCTION_TASK();
    return std::make_shared<ResourceBarriersVK>(barriers);
}

ResourceBarriersVK::ResourceBarriersVK(const Set& barriers)
    : ResourceBarriers(barriers)
{
    META_FUNCTION_TASK();
    for(const ResourceBarrier barrier : barriers)
    {
        AddNativeResourceBarrier(barrier.GetId(), barrier.GetStateChange());
    }
}

ResourceBarriers::AddResult ResourceBarriersVK::AddStateChange(const ResourceBarrier::Id& id, const ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    const auto lock_guard  = ResourceBarriers::Lock();
    const AddResult result = ResourceBarriers::AddStateChange(id, state_change);

    switch (result)
    {
    case AddResult::Added:    AddNativeResourceBarrier(id, state_change); break;
    case AddResult::Updated:  UpdateNativeResourceBarrier(id, state_change); break;
    case AddResult::Existing: break;
    default: META_UNEXPECTED_ARG_RETURN(result, result);
    }

    return result;
}

bool ResourceBarriersVK::Remove(const ResourceBarrier::Id& id)
{
    META_FUNCTION_TASK();
    const auto lock_guard = ResourceBarriers::Lock();
    if (!ResourceBarriers::Remove(id))
        return false;

    // TODO: Remove native memory barriers

    id.GetResource().Disconnect(*this);
    return true;
}

void ResourceBarriersVK::OnResourceReleased(Resource& resource)
{
    META_FUNCTION_TASK();
    RemoveTransition(resource);
}

void ResourceBarriersVK::AddNativeResourceBarrier(const ResourceBarrier::Id& id, const ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    META_UNUSED(state_change);

    id.GetResource().Connect(*this);

    // TODO: Add native memory barrier
}

void ResourceBarriersVK::UpdateNativeResourceBarrier(const ResourceBarrier::Id& id, const ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    META_UNUSED(id);
    META_UNUSED(state_change);

    META_FUNCTION_NOT_IMPLEMENTED();
    // TODO: Update native memory barrier
}

} // namespace Methane::Graphics
