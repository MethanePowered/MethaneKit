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

FILE: Methane/Graphics/Metal/ReleasePoolMT.mm
Metal GPU release pool for deferred objects release.

******************************************************************************/

#include "ReleasePoolMT.hh"
#include "BufferMT.hh"
#include "TextureMT.hh"

#include <Methane/Graphics/RenderContextBase.h>
#include <Methane/Instrumentation.h>

#import <Metal/Metal.h>

namespace Methane::Graphics
{

struct ReleasePoolMT::BufferContainerMT : ReleasePoolMT::IResourceContainerMT
{
    id <MTLBuffer> buffer;

    BufferContainerMT(const BufferMT& buffer) : buffer(buffer.GetNativeBuffer())
    { }
};

struct ReleasePoolMT::TextureContainerMT : ReleasePoolMT::IResourceContainerMT
{
    id <MTLTexture> texture;

    TextureContainerMT(const TextureMT& texture) : texture(texture.GetNativeTexture())
    { }
};

UniquePtr<ReleasePoolMT::IResourceContainerMT> ReleasePoolMT::IResourceContainerMT::Create(ResourceMT& resource)
{
    switch (resource.GetResourceType())
    {
    case Resource::Type::Buffer:
        return std::make_unique<BufferContainerMT>(static_cast<BufferMT&>(resource));
    case Resource::Type::Texture:
        return std::make_unique<TextureContainerMT>(static_cast<TextureMT&>(resource));
    case Resource::Type::Sampler:
        return nullptr;
    default:
        assert(0);
    }
    return nullptr;
}

Ptr<ReleasePool> ReleasePool::Create()
{
    META_FUNCTION_TASK();
    return std::make_shared<ReleasePoolMT>();
}

void ReleasePoolMT::AddResource(ResourceBase& resource)
{
    META_FUNCTION_TASK();
    ResourceMT& resource_mt = static_cast<ResourceMT&>(resource);
    if (resource_mt.GetContextBase().GetType() == Context::Type::Render)
    {
        RenderContextBase& render_context = static_cast<RenderContextBase&>(resource_mt.GetContextBase());
        if (m_frame_resources.size() != render_context.GetSettings().frame_buffers_count)
            m_frame_resources.resize(render_context.GetSettings().frame_buffers_count);

        const uint32_t frame_index = render_context.GetFrameBufferIndex();
        m_frame_resources[frame_index].emplace_back(IResourceContainerMT::Create(resource_mt));
    }
    else
    {
        m_misc_resources.emplace_back(IResourceContainerMT::Create(resource_mt));
    }
}

void ReleasePoolMT::ReleaseAllResources()
{
    META_FUNCTION_TASK();
    for(MTLResourceContainers& frame_resources : m_frame_resources)
    {
        frame_resources.clear();
    }
    m_misc_resources.clear();
}

void ReleasePoolMT::ReleaseFrameResources(uint32_t frame_index)
{
    META_FUNCTION_TASK();
    if (frame_index >= m_frame_resources.size())
        return;

    m_frame_resources[frame_index].clear();
}

} // namespace Methane::Graphics

