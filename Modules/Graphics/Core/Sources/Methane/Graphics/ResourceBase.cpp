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

FILE: Methane/Graphics/ResourceBase.cpp
Base implementation of the resource interface.

******************************************************************************/

#include "ResourceBase.h"
#include "TextureBase.h"
#include "ContextBase.h"

#include <Methane/Graphics/Resource.h>
#include <Methane/Instrumentation.h>

#include <cassert>
#include <sstream>
#include <utility>

namespace Methane::Graphics
{

Resource::Location::Location(Ptr<Resource> sp_resource, Data::Size offset)
    : m_sp_resource(std::move(sp_resource))
    , m_offset(offset)
{
    if (!m_sp_resource)
        throw std::invalid_argument("Can not create Resource Location for an empty resource.");
}

std::string Resource::GetTypeName(Type type) noexcept
{
    META_FUNCTION_TASK();
    switch (type)
    {
    case Resource::Type::Buffer:  return "Buffer";
    case Resource::Type::Texture: return "Texture";
    case Resource::Type::Sampler: return "Sampler";
    default:                      assert(0);
    }
    return "Unknown";
}

std::string Resource::Usage::ToString(Usage::Value usage) noexcept
{
    META_FUNCTION_TASK();
    switch (usage)
    {
    case Resource::Usage::ShaderRead:   return "Shader Read";
    case Resource::Usage::ShaderWrite:  return "Shader Write";
    case Resource::Usage::RenderTarget: return "Render Target";
    case Resource::Usage::Addressable:  return "Addressable";
    default:                            assert(0);
    }
    return "Unknown";
}

std::string Resource::Usage::ToString(Usage::Mask usage_mask) noexcept
{
    META_FUNCTION_TASK();

    std::stringstream names_ss;
    bool first_usage = true;

    for (Usage::Value usage : Usage::values)
    {
        if (!(usage & usage_mask))
            continue;

        if (!first_usage)
        {
            names_ss << ", ";
        }

        names_ss << Usage::ToString(usage);
        first_usage = false;
    }

    return names_ss.str();
}

Resource::Descriptor::Descriptor(DescriptorHeap& in_heap, Data::Index in_index)
    : heap(in_heap)
    , index(in_index)
{
    META_FUNCTION_TASK();
}
    
bool Resource::Location::operator==(const Location& other) const noexcept
{
    return std::tie(m_sp_resource, m_offset) ==
           std::tie(other.m_sp_resource, other.m_offset);
}

Resource::SubResource::SubResource(Data::Bytes&& data, Index index) noexcept
    : Data::Chunk(std::move(data))
    , index(std::move(index))
{
    META_FUNCTION_TASK();
}

Resource::SubResource::SubResource(SubResource&& other) noexcept
    : Data::Chunk(std::move(other))
    , index(other.index)
{
    META_FUNCTION_TASK();
}

Resource::SubResource::SubResource(const SubResource& other) noexcept
    : Data::Chunk(other)
    , index(other.index)
{
    META_FUNCTION_TASK();
}

Resource::SubResource::SubResource(Data::ConstRawPtr p_data, Data::Size size, Index index) noexcept
    : Data::Chunk(p_data, size)
    , index(std::move(index))
{
    META_FUNCTION_TASK();
}

Resource::SubResource::Index::Index(uint32_t depth_slice, uint32_t array_index, uint32_t mip_level) noexcept
    : depth_slice(depth_slice)
    , array_index(array_index)
    , mip_level(mip_level)
{
    META_FUNCTION_TASK();
}

Resource::SubResource::Index Resource::SubResource::Index::FromRawIndex(uint32_t raw_index, uint32_t depth, uint32_t mip_levels_count) noexcept
{
    META_FUNCTION_TASK();
    const uint32_t array_and_depth_index = raw_index / mip_levels_count;
    return Index(array_and_depth_index % depth, array_and_depth_index / depth, raw_index % mip_levels_count);
}

uint32_t Resource::SubResource::Index::GetRawIndex(uint32_t depth, uint32_t mip_levels_count) const noexcept
{
    META_FUNCTION_TASK();
    return (array_index * depth + depth_slice) * mip_levels_count + mip_level;
}

ResourceBase::ResourceBase(Type type, Usage::Mask usage_mask, ContextBase& context, DescriptorByUsage descriptor_by_usage)
    : m_type(type)
    , m_usage_mask(usage_mask)
    , m_context(context)
    , m_descriptor_by_usage(std::move(descriptor_by_usage))
{
    META_FUNCTION_TASK();

    for (auto& usage_and_descriptor : m_descriptor_by_usage)
    {
        usage_and_descriptor.second.heap.ReplaceResource(*this, usage_and_descriptor.second.index);
    }
}

ResourceBase::~ResourceBase()
{
    META_FUNCTION_TASK();

    for (const auto& usage_and_descriptor : m_descriptor_by_usage)
    {
        usage_and_descriptor.second.heap.RemoveResource(usage_and_descriptor.second.index);
    }
}

void ResourceBase::InitializeDefaultDescriptors()
{
    META_FUNCTION_TASK();

    for (Usage::Value usage : Usage::primary_values)
    {
        if (!(m_usage_mask & usage))
            continue;

        auto descriptor_by_usage_it = m_descriptor_by_usage.find(usage);
        if (descriptor_by_usage_it == m_descriptor_by_usage.end())
        {
            // Create default resource descriptor by usage
            const DescriptorHeap::Type heap_type = GetDescriptorHeapTypeByUsage(usage);
            DescriptorHeap& heap = m_context.GetResourceManager().GetDescriptorHeap(heap_type);
            m_descriptor_by_usage.emplace(usage, Descriptor(heap, heap.AddResource(*this)));
        }
    }
}

const Resource::Descriptor& ResourceBase::GetDescriptor(Usage::Value usage) const
{
    META_FUNCTION_TASK();

    auto descriptor_by_usage_it = m_descriptor_by_usage.find(usage);
    if (descriptor_by_usage_it == m_descriptor_by_usage.end())
    {
        throw std::runtime_error("Resource \"" + GetName() + "\" does not support \"" + Usage::ToString(usage) + "\" usage");
    }
    return descriptor_by_usage_it->second;
}

void ResourceBase::SetData(const SubResources& sub_resources)
{
    META_FUNCTION_TASK();

    if (sub_resources.empty())
    {
        throw std::invalid_argument("Can not set buffer data from empty sub-resources.");
    }

    Data::Size sub_resources_data_size = 0u;
    for(const SubResource& sub_resource : sub_resources)
    {
        if (!sub_resource.p_data || !sub_resource.size)
        {
            throw std::invalid_argument("Can not set empty subresource data to buffer.");
        }
        sub_resources_data_size += sub_resource.size;
    }

    const Data::Size reserved_data_size = GetDataSize(Data::MemoryState::Reserved);
    if (sub_resources_data_size > reserved_data_size)
    {
        throw std::runtime_error("Can not set more data (" + std::to_string(sub_resources_data_size) +
                                 ") than allocated buffer size (" + std::to_string(reserved_data_size) + ").");
    }

    m_initialized_data_size = sub_resources_data_size;
}

DescriptorHeap::Type ResourceBase::GetDescriptorHeapTypeByUsage(ResourceBase::Usage::Value resource_usage) const
{
    META_FUNCTION_TASK();

    switch (resource_usage)
    {
    case Resource::Usage::ShaderRead:
    {
        return (m_type == Type::Sampler)
                ? DescriptorHeap::Type::Samplers
                : DescriptorHeap::Type::ShaderResources;
    }
    case Resource::Usage::ShaderWrite:
    case Resource::Usage::RenderTarget:
        return (m_type == Type::Texture && static_cast<const TextureBase&>(*this).GetSettings().type == Texture::Type::DepthStencilBuffer)
                ? DescriptorHeap::Type::DepthStencil
                : DescriptorHeap::Type::RenderTargets;
    case Resource::Usage::Unknown:
        return DescriptorHeap::Type::Undefined;
    default:
        throw std::runtime_error("Resource usage value (" + std::to_string(static_cast<uint32_t>(resource_usage)) + ") does not map to descriptor heap.");
    }
}

const Resource::Descriptor& ResourceBase::GetDescriptorByUsage(Usage::Value usage) const
{
    META_FUNCTION_TASK();

    auto descriptor_by_usage_it = m_descriptor_by_usage.find(usage);
    if (descriptor_by_usage_it == m_descriptor_by_usage.end())
    {
        throw std::runtime_error("Resource \"" + GetName() + "\" does not have descriptor for usage \"" + Usage::ToString(usage) + "\"");
    }

    return descriptor_by_usage_it->second;
}

DescriptorHeap::Types ResourceBase::GetUsedDescriptorHeapTypes() const noexcept
{
    META_FUNCTION_TASK();

    DescriptorHeap::Types heap_types;
    for (auto usage_and_descriptor : m_descriptor_by_usage)
    {
        heap_types.insert(usage_and_descriptor.second.heap.GetSettings().type);
    }
    return heap_types;
}

void ResourceBase::SetState(State state, Barriers& out_barriers)
{
    META_FUNCTION_TASK();

    if (m_state == state)
        return;

    if (m_state != State::Common)
    {
        assert(state != State::Common);
        out_barriers.emplace_back(Barrier{ Barrier::Type::Transition, *this, m_state, state });
    }

    m_state = state;
}

} // namespace Methane::Graphics
