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

ResourceBase::Barrier::Barrier(Type type, Resource& resource, State state_before, State state_after)
    : type(type)
    , resource(resource)
    , state_before(state_before)
    , state_after(state_after)
{
    META_FUNCTION_TASK();
}

Ptr<ResourceBase::Barriers> ResourceBase::Barriers::CreateTransition(const Refs<Resource>& resources, State state_before, State state_after)
{
    META_FUNCTION_TASK();
    std::vector<Barrier> resource_barriers;
    resource_barriers.reserve(resources.size());
    for (const Ref<Resource>& resource_ref : resources)
    {
        resource_barriers.emplace_back(Barrier{
            ResourceBase::Barrier::Type::Transition,
            resource_ref.get(),
            state_before,
            state_after
        });
    }
    return Barriers::Create(resource_barriers);
}

ResourceBase::Barriers::Barriers(std::vector<Barrier> barriers)
    : m_barriers(std::move(barriers))
{
    META_FUNCTION_TASK();
}

const ResourceBase::Barrier& ResourceBase::Barriers::Add(Barrier::Type type, Resource& resource, State state_before, State state_after)
{
    META_FUNCTION_TASK();
    m_barriers.emplace_back(type, resource, state_before, state_after);
    return m_barriers.back();
}


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
    case Resource::Usage::CpuReadback:  return "CPU Read-back";
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

Resource::SubResource::Count::Count(Data::Size depth, Data::Size array_size, Data::Size mip_levels_count)
    : depth(depth)
    , array_size(array_size)
    , mip_levels_count(mip_levels_count)
{
    META_FUNCTION_TASK();
    if (depth == 0 || array_size == 0 || mip_levels_count == 0)
        throw std::invalid_argument("Subresource count can not be zero.");
}

Data::Size Resource::SubResource::Count::GetRawCount() const noexcept
{
    META_FUNCTION_TASK();
    return depth * array_size * mip_levels_count;
}

void Resource::SubResource::Count::operator+=(const Index& index) noexcept
{
    depth            = std::max(depth,            index.depth_slice + 1u);
    array_size       = std::max(array_size,       index.array_index + 1u);
    mip_levels_count = std::max(mip_levels_count, index.mip_level   + 1u);
}

bool Resource::SubResource::Count::operator==(const Count& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(depth, array_size, mip_levels_count) ==
           std::tie(other.depth, other.array_size, other.mip_levels_count);
}

bool Resource::SubResource::Count::operator<(const Count& other) const noexcept
{
    META_FUNCTION_TASK();
    return GetRawCount() < other.GetRawCount();
}

bool Resource::SubResource::Count::operator>=(const Count& other) const noexcept
{
    META_FUNCTION_TASK();
    return GetRawCount() >= other.GetRawCount();
}

Resource::SubResource::Count::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    std::stringstream ss;
    ss << "count(d:" << std::to_string(depth)
       <<     ", a:" << std::to_string(array_size)
       <<     ", m:" << std::to_string(mip_levels_count) << ")";
    return ss.str();
}

Resource::SubResource::Index::Index(Data::Index depth_slice, Data::Index array_index, Data::Index mip_level) noexcept
    : depth_slice(depth_slice)
    , array_index(array_index)
    , mip_level(mip_level)
{
    META_FUNCTION_TASK();
}

Resource::SubResource::Index Resource::SubResource::Index::FromRawIndex(Data::Index raw_index, const Count& count)
{
    META_FUNCTION_TASK();
    const Data::Size raw_count = count.GetRawCount();
    if (raw_index >= raw_count)
        throw std::invalid_argument("Subresource raw index (" + std::to_string(raw_index) + " ) is out of range (count: " + std::to_string(raw_count) + ").");

    const uint32_t array_and_depth_index = raw_index / count.mip_levels_count;
    return Index(array_and_depth_index % count.depth, array_and_depth_index / count.depth, raw_index % count.mip_levels_count);
}

Data::Index Resource::SubResource::Index::GetRawIndex(const Count& count) const noexcept
{
    META_FUNCTION_TASK();
    return (array_index * count.depth + depth_slice) * count.mip_levels_count + mip_level;
}

bool Resource::SubResource::Index::operator==(const Index& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(depth_slice, array_index, mip_level) ==
           std::tie(other.depth_slice, other.array_index, other.mip_level);
}

bool Resource::SubResource::Index::operator<(const Index& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(depth_slice, array_index, mip_level) <
           std::tie(other.depth_slice, other.array_index, other.mip_level);
}

bool Resource::SubResource::Index::operator<(const Count& count) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(depth_slice, array_index, mip_level) <
           std::tie(count.depth, count.array_size, count.mip_levels_count);
}

bool Resource::SubResource::Index::operator>=(const Count& count) const noexcept
{
    META_FUNCTION_TASK();
    return !operator<(count);
}

Resource::SubResource::Index::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    std::stringstream ss;
    ss << "index(d:" << std::to_string(depth_slice)
       <<     ", a:" << std::to_string(array_index)
       <<     ", m:" << std::to_string(mip_level) << ")";
    return ss.str();
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
        if (m_subresource_count_constant)
        {
            if (sub_resource.index >= m_subresource_count)
                throw std::invalid_argument("Subresource " + static_cast<std::string>(sub_resource.index) +
                                            " exceeds available subresources range " + static_cast<std::string>(m_subresource_count));
        }
        else
        {
            m_subresource_count += sub_resource.index;
        }
    }

    const Data::Size reserved_data_size = GetDataSize(Data::MemoryState::Reserved);
    if (sub_resources_data_size > reserved_data_size)
    {
        throw std::runtime_error("Can not set more data (" + std::to_string(sub_resources_data_size) +
                                 ") than allocated buffer size (" + std::to_string(reserved_data_size) + ").");
    }

    m_initialized_data_size = sub_resources_data_size;

    FillSubresourceRanges();
}

Resource::SubResource ResourceBase::GetData(const BytesRange&)
{
    META_FUNCTION_TASK();
    throw std::logic_error("Reading data is not allowed for this type of resource.");
}

const Resource::BytesRange& ResourceBase::GetSubresourceDataRange(const SubResource::Index& subresource_index) const
{
    META_FUNCTION_TASK();
    if (subresource_index >= m_subresource_count)
        throw std::invalid_argument("Sub-resource "     + static_cast<std::string>(subresource_index) +
                                    " is out of range " + static_cast<std::string>(m_subresource_count));

    return m_subresource_ranges[subresource_index.GetRawIndex()];
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

void ResourceBase::SetState(State state, Ptr<Barriers>& out_barriers)
{
    META_FUNCTION_TASK();

    if (m_state == state)
        return;

    if (m_state != State::Common)
    {
        if (state == State::Common)
        {
            throw std::logic_error("Resource can not be transitioned to \"Common\" state.");
        }
        if (!out_barriers)
        {
            out_barriers = Barriers::Create();
        }
        out_barriers->Add(Barrier::Type::Transition, *this, m_state, state);
    }

    m_state = state;
}

void ResourceBase::SetSubresourceCount(const SubResource::Count& sub_resource_count)
{
    META_FUNCTION_TASK();

    m_subresource_count_constant = true;
    m_subresource_count = sub_resource_count;
    m_subresource_ranges.clear();

    FillSubresourceRanges();
}

Data::Size ResourceBase::GetSubresourceDataSize(const SubResource::Index& subresource_index) const
{
    META_FUNCTION_TASK();
    static const SubResource::Index s_zero_index;
    if (subresource_index == s_zero_index && m_subresource_count.GetRawCount() == 1)
        return GetDataSize();

    throw std::invalid_argument("Subresource size is undefined, must be provided by super class override.");
}

void ResourceBase::FillSubresourceRanges()
{
    const Data::Size curr_subresource_raw_count = m_subresource_count.GetRawCount();
    const Data::Size prev_subresource_raw_count = static_cast<Data::Size>(m_subresource_ranges.size());
    if (curr_subresource_raw_count == prev_subresource_raw_count)
        return;

    m_subresource_ranges.reserve(curr_subresource_raw_count);
    Data::Size subresource_offset = m_subresource_ranges.empty() ? 0u : m_subresource_ranges.back().GetEnd();
    for (Data::Index subresource_raw_index = prev_subresource_raw_count; subresource_raw_index < curr_subresource_raw_count; ++subresource_raw_index)
    {
        const SubResource::Index subresource_index = SubResource::Index::FromRawIndex(subresource_raw_index, m_subresource_count);
        const Data::Size subresource_data_size = GetSubresourceDataSize(subresource_index);

        m_subresource_ranges.emplace_back(subresource_offset, subresource_offset + subresource_data_size);
        subresource_offset += subresource_data_size;
    }
}

} // namespace Methane::Graphics
