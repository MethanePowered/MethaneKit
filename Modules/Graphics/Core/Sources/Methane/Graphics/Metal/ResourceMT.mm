/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/ResourceMT.mm
Metal implementation of the resource interface.

******************************************************************************/

#include "ResourceMT.hh"
#include "ContextMT.h"
#include "BufferMT.hh"
#include "TextureMT.hh"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

#include <vector>

namespace Methane::Graphics
{

struct ResourceContainerMT
{
    std::vector<id<MTLBuffer>>  buffers;
    std::vector<id<MTLTexture>> textures;
};

Ptr<ResourceBase::ReleasePool> ResourceBase::ReleasePool::Create()
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ResourceMT::ReleasePoolMT>();
}

ResourceMT::ReleasePoolMT::ReleasePoolMT()
    : ResourceBase::ReleasePool()
    , m_sp_mtl_resources(new ResourceContainerMT())
{
    ITT_FUNCTION_TASK();
}

void ResourceMT::ReleasePoolMT::AddResource(ResourceBase& resource)
{
    ITT_FUNCTION_TASK();

    switch(resource.GetResourceType())
    {
        case Resource::Type::Buffer:
            m_sp_mtl_resources->buffers.push_back(static_cast<BufferMT&>(resource).GetNativeBuffer());
            break;
        case Resource::Type::Texture:
            m_sp_mtl_resources->textures.push_back(static_cast<TextureMT&>(resource).GetNativeTexture());
            break;
        case Resource::Type::Sampler:
            // Nothing to do here
            break;
    }
}

void ResourceMT::ReleasePoolMT::ReleaseResources()
{
    ITT_FUNCTION_TASK();

    assert(!!m_sp_mtl_resources);
    for(id<MTLBuffer>& mtl_buffer : m_sp_mtl_resources->buffers)
    {
        [mtl_buffer release];
    }
    for(id<MTLTexture>& mtl_texture : m_sp_mtl_resources->textures)
    {
        [mtl_texture release];
    }
    m_sp_mtl_resources.reset(new ResourceContainerMT());
}

ResourceMT::ResourceMT(Type type, Usage::Mask usage_mask, ContextBase& context, const DescriptorByUsage& descriptor_by_usage)
    : ResourceBase(type, usage_mask, context, descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
}

IContextMT& ResourceMT::GetContextMT() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<IContextMT&>(GetContext());
}

} // namespace Methane::Graphics
