/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/ResourceBase.cpp
Base implementation of the resource interface.

******************************************************************************/

#include "ResourceBase.h"
#include "TextureBase.h"
#include "ContextBase.h"
#include "Formatters.hpp"

#include <Methane/Graphics/Resource.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <fmt/format.h>

#include <sstream>
#include <utility>
#include <algorithm>

namespace Methane::Graphics
{

ResourceBase::Barrier::Id::Id(Type type, Resource& resource)
    : type(type)
    , resource(resource)
{
    META_FUNCTION_TASK();
}

bool ResourceBase::Barrier::Id::operator<(const Id& other) const noexcept
{
    META_FUNCTION_TASK();
    const Resource* p_this_resource  = std::addressof(resource);
    const Resource* p_other_resource = std::addressof(other.resource);
    return std::tie(type, p_this_resource) < std::tie(other.type, p_other_resource);
}

bool ResourceBase::Barrier::Id::operator==(const Id& other) const noexcept
{
    META_FUNCTION_TASK();
    const Resource* p_this_resource  = std::addressof(resource);
    const Resource* p_other_resource = std::addressof(other.resource);
    return std::tie(type, p_this_resource) == std::tie(other.type, p_other_resource);
}

bool ResourceBase::Barrier::Id::operator!=(const Id& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

ResourceBase::Barrier::StateChange::StateChange(State before, State after)
    : before(before)
    , after(after)
{
    META_FUNCTION_TASK();
}

bool ResourceBase::Barrier::StateChange::operator<(const StateChange& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(before, after) < std::tie(other.before, other.after);
}

bool ResourceBase::Barrier::StateChange::operator==(const StateChange& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(before, after) == std::tie(other.before, other.after);
}

bool ResourceBase::Barrier::StateChange::operator!=(const StateChange& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

ResourceBase::Barrier::Barrier(Id id, StateChange state_change)
    : id(std::move(id))
    , state_change(std::move(state_change))
{
    META_FUNCTION_TASK();
}

ResourceBase::Barrier::Barrier(Type type, Resource& resource, State state_before, State state_after)
    : Barrier(Id(type, resource), StateChange(state_before, state_after))
{
    META_FUNCTION_TASK();
}


bool ResourceBase::Barrier::operator<(const Barrier& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(id, state_change) < std::tie(other.id, other.state_change);
}

bool ResourceBase::Barrier::operator==(const Barrier& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(id, state_change) == std::tie(other.id, other.state_change);
}

bool ResourceBase::Barrier::operator!=(const Barrier& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

ResourceBase::Barrier::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    std::stringstream ss;
    ss << "Resource \"" << id.resource.GetName()
       << "\" " << GetTypeName(id.type)
       << " barrier from " << GetStateName(state_change.before)
       << " to " << GetStateName(state_change.after)
       << " state";
    return ss.str();
}

std::string ResourceBase::Barrier::GetTypeName(Type type)
{
    META_FUNCTION_TASK();
    switch(type)
    {
    case Type::Transition: return "Transition";
    default: META_UNEXPECTED_ENUM_ARG_RETURN(type, "");
    }
}

Ptr<ResourceBase::Barriers> ResourceBase::Barriers::CreateTransition(const Refs<Resource>& resources, State state_before, State state_after)
{
    META_FUNCTION_TASK();
    std::set<Barrier> resource_barriers;
    for (const Ref<Resource>& resource_ref : resources)
    {
        resource_barriers.emplace(Barrier{
            ResourceBase::Barrier::Type::Transition,
            resource_ref.get(),
            state_before,
            state_after
        });
    }
    return Barriers::Create(resource_barriers);
}

ResourceBase::Barriers::Barriers(const Set& barriers)
{
    META_FUNCTION_TASK();
    std::transform(barriers.begin(), barriers.end(), std::inserter(m_barriers_map, m_barriers_map.begin()),
        [](const Barrier& barrier)
        {
            return std::pair<Barrier::Id, Barrier::StateChange>{ barrier.id, barrier.state_change };
        }
    );
}

ResourceBase::Barriers::Set ResourceBase::Barriers::GetSet() const noexcept
{
    META_FUNCTION_TASK();
    Set barriers;
    std::transform(m_barriers_map.begin(), m_barriers_map.end(), std::inserter(barriers, barriers.begin()),
        [](const auto& barrier_pair)
        {
           return Barrier(barrier_pair.first, barrier_pair.second);
        }
    );
    return barriers;
}

bool ResourceBase::Barriers::Has(Barrier::Type type, Resource& resource, State before, State after)
{
    META_FUNCTION_TASK();
    const auto barrier_it = m_barriers_map.find(Barrier::Id(type, resource));
    if (barrier_it == m_barriers_map.end())
        return false;

    return barrier_it->second == Barrier::StateChange(before, after);
}

bool ResourceBase::Barriers::HasTransition(Resource& resource, State before, State after)
{
    META_FUNCTION_TASK();
    return Has(Barrier::Type::Transition, resource, before, after);
}

bool ResourceBase::Barriers::Add(Barrier::Type type, Resource& resource, State before, State after)
{
    META_FUNCTION_TASK();
    return Add(Barrier::Id(type, resource), Barrier::StateChange(before, after));
}

bool ResourceBase::Barriers::AddTransition(Resource& resource, State before, State after)
{
    META_FUNCTION_TASK();
    return Add(Barrier::Id(Barrier::Type::Transition, resource), Barrier::StateChange(before, after));
}

bool ResourceBase::Barriers::Add(const Barrier::Id& id, const Barrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    const auto emplace_result = m_barriers_map.emplace(id, state_change);
    if (emplace_result.second)
        return true;
    else if (emplace_result.first->second == state_change)
        return false;

    emplace_result.first->second = state_change;
    return true;
}

ResourceBase::Barriers::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    std::stringstream ss;
    for(auto barrier_pair_it = m_barriers_map.begin(); barrier_pair_it != m_barriers_map.end(); ++barrier_pair_it)
    {
        ss << "  - " << static_cast<std::string>(Barrier(barrier_pair_it->first, barrier_pair_it->second));
        if (barrier_pair_it != std::prev(m_barriers_map.end()))
            ss << ";" << std::endl;
        else
            ss << ".";
    }
    return ss.str();
}

Resource::Location::Location(Ptr<Resource> resource_ptr, Data::Size offset)
    : m_resource_ptr(std::move(resource_ptr))
    , m_offset(offset)
{
    META_CHECK_ARG_NOT_NULL_DESCR(m_resource_ptr, "can not create resource location for an empty resource");
}

std::string Resource::GetTypeName(Type type)
{
    META_FUNCTION_TASK();
    switch (type)
    {
    case Resource::Type::Buffer:  return "Buffer";
    case Resource::Type::Texture: return "Texture";
    case Resource::Type::Sampler: return "Sampler";
    default:                      META_UNEXPECTED_ENUM_ARG_RETURN(type, "Unknown");
    }
}

std::string Resource::Usage::ToString(Usage::Value usage)
{
    META_FUNCTION_TASK();
    switch (usage)
    {
    case Resource::Usage::ShaderRead:   return "Shader Read";
    case Resource::Usage::ShaderWrite:  return "Shader Write";
    case Resource::Usage::RenderTarget: return "Render Target";
    case Resource::Usage::ReadBack:     return "Read Back";
    case Resource::Usage::Addressable:  return "Addressable";
    default:                            META_UNEXPECTED_ENUM_ARG_RETURN(usage, "Unknown");
    }
}

std::string Resource::Usage::ToString(Usage::Mask usage_mask)
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
    return std::tie(m_resource_ptr, m_offset) ==
           std::tie(other.m_resource_ptr, other.m_offset);
}

Resource::SubResource::SubResource(SubResource&& other) noexcept
    : Data::Chunk(std::move(static_cast<Data::Chunk&&>(other)))
    , index(std::move(other.index))
    , data_range(std::move(other.data_range))
{
    META_FUNCTION_TASK();
}

Resource::SubResource::SubResource(const SubResource& other) noexcept
    : Data::Chunk(other)
    , index(other.index)
    , data_range(other.data_range)
{
    META_FUNCTION_TASK();
}

Resource::SubResource::SubResource(Data::Bytes&& data, Index index, std::optional<BytesRange> data_range) noexcept
    : Data::Chunk(std::move(data))
    , index(std::move(index))
    , data_range(std::move(data_range))
{
    META_FUNCTION_TASK();
}

Resource::SubResource::SubResource(Data::ConstRawPtr p_data, Data::Size size, Index index, std::optional<BytesRange> data_range) noexcept
    : Data::Chunk(p_data, size)
    , index(std::move(index))
    , data_range(std::move(data_range))
{
    META_FUNCTION_TASK();
}

Resource::SubResource::Count::Count(Data::Size depth, Data::Size array_size, Data::Size mip_levels_count)
    : depth(depth)
    , array_size(array_size)
    , mip_levels_count(mip_levels_count)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_ZERO_DESCR(depth, "subresource count can not be zero");
    META_CHECK_ARG_NOT_ZERO_DESCR(array_size, "subresource count can not be zero");
    META_CHECK_ARG_NOT_ZERO_DESCR(mip_levels_count, "subresource count can not be zero");
}

Data::Size Resource::SubResource::Count::GetRawCount() const noexcept
{
    META_FUNCTION_TASK();
    return depth * array_size * mip_levels_count;
}

void Resource::SubResource::Count::operator+=(const Index& index) noexcept
{
    depth            = std::max(depth,            index.depth_slice + 1U);
    array_size       = std::max(array_size,       index.array_index + 1U);
    mip_levels_count = std::max(mip_levels_count, index.mip_level   + 1U);
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

Resource::SubResource::Count::operator Resource::SubResource::Index() const noexcept
{
    META_FUNCTION_TASK();
    return Resource::SubResource::Index(*this);
}

Resource::SubResource::Count::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    return fmt::format("count(d:{}, a:{}, m:{})", depth, array_size, mip_levels_count);
}

Resource::SubResource::Index::Index(Data::Index depth_slice, Data::Index array_index, Data::Index mip_level) noexcept
    : depth_slice(depth_slice)
    , array_index(array_index)
    , mip_level(mip_level)
{
    META_FUNCTION_TASK();
}

Resource::SubResource::Index::Index(Data::Index raw_index, const Count& count)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(raw_index, count.GetRawCount());

    const uint32_t array_and_depth_index = raw_index / count.mip_levels_count;
    depth_slice = array_and_depth_index % count.depth;
    array_index = array_and_depth_index / count.depth;
    mip_level   = raw_index % count.mip_levels_count;
}

Resource::SubResource::Index::Index(const Resource::SubResource::Count& count)
    : depth_slice(count.depth)
    , array_index(count.array_size)
    , mip_level(count.mip_levels_count)
{
    META_FUNCTION_TASK();
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

bool Resource::SubResource::Index::operator>=(const Index& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator<(other);
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
    return fmt::format("index(d:{}, a:{}, m:{})", depth_slice, array_index, mip_level);
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
    META_CHECK_ARG_DESCR(usage, descriptor_by_usage_it != m_descriptor_by_usage.end(),
                         fmt::format("resource '{}' does not support '{}' usage", GetName(), Usage::ToString(usage)));
    return descriptor_by_usage_it->second;
}

void ResourceBase::SetData(const SubResources& sub_resources)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_EMPTY_DESCR(sub_resources, "can not set buffer data from empty sub-resources");

    Data::Size sub_resources_data_size = 0U;
    for(const SubResource& sub_resource : sub_resources)
    {
        META_CHECK_ARG_NAME_DESCR("sub_resource", sub_resource.p_data && sub_resource.size, "can not set empty subresource data to buffer");
        sub_resources_data_size += sub_resource.size;

        if (m_sub_resource_count_constant)
        {
            META_CHECK_ARG_LESS(sub_resource.index, m_sub_resource_count);
        }
        else
        {
            m_sub_resource_count += sub_resource.index;
        }
    }

    const Data::Size reserved_data_size = GetDataSize(Data::MemoryState::Reserved);
    META_UNUSED(reserved_data_size);

    META_CHECK_ARG_LESS_DESCR(sub_resources_data_size, reserved_data_size + 1, "can not set more data than allocated buffer size");
    m_initialized_data_size = sub_resources_data_size;

    if (!m_sub_resource_count_constant)
    {
        FillSubresourceSizes();
    }
}

Resource::SubResource ResourceBase::GetData(const SubResource::Index&, const std::optional<BytesRange>&)
{
    META_FUNCTION_NOT_IMPLEMENTED_RETURN_DESCR(Resource::SubResource(), "reading data is not allowed for this type of resource");
}

Context& ResourceBase::GetContext() noexcept
{
    META_FUNCTION_TASK();
    return m_context;
}

Data::Size ResourceBase::GetSubResourceDataSize(const SubResource::Index& sub_resource_index) const
{
    META_FUNCTION_TASK();
    ValidateSubResource(sub_resource_index);
    return m_sub_resource_sizes[sub_resource_index.GetRawIndex(m_sub_resource_count)];
}

DescriptorHeap::Type ResourceBase::GetDescriptorHeapTypeByUsage(ResourceBase::Usage::Value resource_usage) const
{
    META_FUNCTION_TASK();
    switch (resource_usage)
    {
    case Resource::Usage::ShaderRead:
        return (m_type == Type::Sampler)
                ? DescriptorHeap::Type::Samplers
                : DescriptorHeap::Type::ShaderResources;

    case Resource::Usage::ShaderWrite:
    case Resource::Usage::RenderTarget:
        return (m_type == Type::Texture && static_cast<const TextureBase&>(*this).GetSettings().type == Texture::Type::DepthStencilBuffer)
                ? DescriptorHeap::Type::DepthStencil
                : DescriptorHeap::Type::RenderTargets;

    default:
        META_UNEXPECTED_ENUM_ARG_DESCR_RETURN(resource_usage, DescriptorHeap::Type::Undefined, "resource usage does not map to descriptor heap");
    }
}

const Resource::Descriptor& ResourceBase::GetDescriptorByUsage(Usage::Value usage) const
{
    META_FUNCTION_TASK();
    auto descriptor_by_usage_it = m_descriptor_by_usage.find(usage);
    META_CHECK_ARG_DESCR(usage, descriptor_by_usage_it != m_descriptor_by_usage.end(),
                         fmt::format("Resource '{}' does not have descriptor for usage '{}'", GetName(), Usage::ToString(usage)));
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

bool ResourceBase::SetState(State state, Ptr<Barriers>& out_barriers)
{
    META_FUNCTION_TASK();
    if (m_state == state)
        return false;

    META_LOG("Resource \"" + GetName() + "\" state changed from " + GetStateName(m_state) + " to " + GetStateName(state));

    if (m_state != State::Common)
    {
        META_CHECK_ARG_NOT_EQUAL_DESCR(state, State::Common, "resource can not be transitioned to 'Common' state");
        if (!out_barriers || !out_barriers->HasTransition(*this, m_state, state))
        {
            out_barriers = Barriers::Create();
        }
        out_barriers->AddTransition(*this, m_state, state);
    }

    m_state = state;
    return true;
}

void ResourceBase::SetSubResourceCount(const SubResource::Count& sub_resource_count)
{
    META_FUNCTION_TASK();

    m_sub_resource_count_constant = true;
    m_sub_resource_count          = sub_resource_count;
    m_sub_resource_sizes.clear();

    FillSubresourceSizes();
}

void ResourceBase::ValidateSubResource(const SubResource& sub_resource) const
{
    META_FUNCTION_TASK();
    ValidateSubResource(sub_resource.index, sub_resource.data_range);

    const Data::Index sub_resource_raw_index = sub_resource.index.GetRawIndex(m_sub_resource_count);
    const Data::Size sub_resource_data_size = m_sub_resource_sizes[sub_resource_raw_index];
    META_UNUSED(sub_resource_data_size);

    if (sub_resource.data_range)
    {
        META_CHECK_ARG_EQUAL_DESCR(sub_resource.size, sub_resource.data_range->GetLength(),
                                   fmt::format("sub-resource {} data size should be equal to the length of data range", sub_resource.index));

        META_CHECK_ARG_LESS_DESCR(sub_resource.size, sub_resource_data_size + 1,
                                  fmt::format("sub-resource {} data size should be less or equal than full resource size", sub_resource.index));
    }
    else
    {
        META_CHECK_ARG_EQUAL_DESCR(sub_resource.size, sub_resource_data_size,
                             fmt::format("Sub-resource {} data size should be equal to full resource size when data range is not specified", sub_resource.index));
    }
}

void ResourceBase::ValidateSubResource(const SubResource::Index& sub_resource_index, const std::optional<BytesRange>& sub_resource_data_range) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(sub_resource_index, m_sub_resource_count);
    if (!sub_resource_data_range)
        return;

    META_CHECK_ARG_NAME_DESCR("sub_resource_data_range", !sub_resource_data_range->IsEmpty(), fmt::format("sub-resource {} data range can not be empty", sub_resource_index));
    const Data::Index sub_resource_raw_index = sub_resource_index.GetRawIndex(m_sub_resource_count);
    META_CHECK_ARG_LESS(sub_resource_raw_index, m_sub_resource_sizes.size());

    const Data::Size sub_resource_data_size = m_sub_resource_sizes[sub_resource_raw_index];
    META_UNUSED(sub_resource_data_size);
    META_CHECK_ARG_LESS_DESCR(sub_resource_data_range->GetEnd(), sub_resource_data_size + 1, fmt::format("sub-resource index {}", sub_resource_index));
}

Data::Size ResourceBase::CalculateSubResourceDataSize(const SubResource::Index& subresource_index) const
{
    META_FUNCTION_TASK();
    static const SubResource::Index s_zero_index;
    META_CHECK_ARG_EQUAL_DESCR(subresource_index, s_zero_index, "subresource size is undefined, must be provided by super class override");

    static const SubResource::Count s_one_count;
    META_CHECK_ARG_EQUAL_DESCR(m_sub_resource_count, s_one_count, "subresource size is undefined, must be provided by super class override");

    return GetDataSize();
}

void ResourceBase::FillSubresourceSizes()
{
    META_FUNCTION_TASK();
    const Data::Size curr_subresource_raw_count = m_sub_resource_count.GetRawCount();
    const Data::Size prev_subresource_raw_count = static_cast<Data::Size>(m_sub_resource_sizes.size());
    if (curr_subresource_raw_count == prev_subresource_raw_count)
        return;

    m_sub_resource_sizes.reserve(curr_subresource_raw_count);
    for (Data::Index subresource_raw_index = prev_subresource_raw_count; subresource_raw_index < curr_subresource_raw_count; ++subresource_raw_index)
    {
        const SubResource::Index subresource_index(subresource_raw_index, m_sub_resource_count);
        m_sub_resource_sizes.emplace_back(CalculateSubResourceDataSize(subresource_index));
    }
}

std::string ResourceBase::GetStateName(State state)
{
    META_FUNCTION_TASK();
    switch(state)
    {
    case State::Common:                  return "Common";
    case State::VertexAndConstantBuffer: return "VertexAndConstantBuffer";
    case State::IndexBuffer:             return "IndexBuffer";
    case State::RenderTarget:            return "RenderTarget";
    case State::UnorderedAccess:         return "UnorderedAccess";
    case State::DepthWrite:              return "DepthWrite";
    case State::DepthRead:               return "DepthRead";
    case State::NonPixelShaderResource:  return "NonPixelShaderResource";
    case State::PixelShaderResource:     return "PixelShaderResource";
    case State::StreamOut:               return "StreamOut";
    case State::IndirectArgument:        return "IndirectArgument";
    case State::CopyDest:                return "CopyDest";
    case State::CopySource:              return "CopySource";
    case State::ResolveDest:             return "ResolveDest";
    case State::ResolveSource:           return "ResolveSource";
    case State::GenericRead:             return "GenericRead";
    case State::Present:                 return "Present";
    case State::Predication:             return "Predication";
    default:                             META_UNEXPECTED_ENUM_ARG_RETURN(state, "");
    }
}

} // namespace Methane::Graphics
