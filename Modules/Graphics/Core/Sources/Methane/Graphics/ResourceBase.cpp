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
#include "CoreFormatters.hpp"

#include <Methane/Graphics/Resource.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <fmt/format.h>
#include <magic_enum.hpp>

#include <sstream>
#include <utility>
#include <algorithm>

namespace Methane::Graphics
{

ResourceBase::Barrier::Id::Id(Type type, const Resource& resource) noexcept
    : m_type(type)
    , m_resource_ref(resource)
{
    META_FUNCTION_TASK();
}

bool ResourceBase::Barrier::Id::operator<(const Id& other) const noexcept
{
    META_FUNCTION_TASK();
    const Resource* p_this_resource  = std::addressof(m_resource_ref.get());
    const Resource* p_other_resource = std::addressof(other.GetResource());
    return std::tie(m_type, p_this_resource) < std::tie(other.m_type, p_other_resource);
}

bool ResourceBase::Barrier::Id::operator==(const Id& other) const noexcept
{
    META_FUNCTION_TASK();
    const Resource* p_this_resource  = std::addressof(m_resource_ref.get());
    const Resource* p_other_resource = std::addressof(other.GetResource());
    return std::tie(m_type, p_this_resource) == std::tie(other.m_type, p_other_resource);
}

bool ResourceBase::Barrier::Id::operator!=(const Id& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

ResourceBase::Barrier::StateChange::StateChange(State before, State after) noexcept
    : m_before(before)
    , m_after(after)
{
    META_FUNCTION_TASK();
}

bool ResourceBase::Barrier::StateChange::operator<(const StateChange& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_before, m_after) < std::tie(other.m_before, other.m_after);
}

bool ResourceBase::Barrier::StateChange::operator==(const StateChange& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_before, m_after) == std::tie(other.m_before, other.m_after);
}

bool ResourceBase::Barrier::StateChange::operator!=(const StateChange& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

ResourceBase::Barrier::Barrier(const Id& id, const StateChange& state_change)
    : m_id(id)
    , m_state_change(state_change)
{
    META_FUNCTION_TASK();
}

ResourceBase::Barrier::Barrier(Type type, const Resource& resource, State state_before, State state_after)
    : Barrier(Id(type, resource), StateChange(state_before, state_after))
{
    META_FUNCTION_TASK();
}


bool ResourceBase::Barrier::operator<(const Barrier& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_id, m_state_change) < std::tie(other.m_id, other.m_state_change);
}

bool ResourceBase::Barrier::operator==(const Barrier& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_id, m_state_change) == std::tie(other.m_id, other.m_state_change);
}

bool ResourceBase::Barrier::operator!=(const Barrier& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

ResourceBase::Barrier::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    return fmt::format("Resource '{}' {} barrier from {} to {} state",
                       m_id.GetResource().GetName(),
                       magic_enum::enum_name(m_id.GetType()),
                       magic_enum::enum_name(m_state_change.GetStateBefore()),
                       magic_enum::enum_name(m_state_change.GetStateAfter()));
}

Ptr<ResourceBase::Barriers> ResourceBase::Barriers::CreateTransition(const Refs<const Resource>& resources, State state_before, State state_after)
{
    META_FUNCTION_TASK();
    std::set<Barrier> resource_barriers;
    for (const Ref<const Resource>& resource_ref : resources)
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
            return std::pair<Barrier::Id, Barrier::StateChange>{ barrier.GetId(), barrier.GetStateChange() };
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

bool ResourceBase::Barriers::Has(Barrier::Type type, const Resource& resource, State before, State after)
{
    META_FUNCTION_TASK();
    const auto barrier_it = m_barriers_map.find(Barrier::Id(type, resource));
    if (barrier_it == m_barriers_map.end())
        return false;

    return barrier_it->second == Barrier::StateChange(before, after);
}

bool ResourceBase::Barriers::HasTransition(const Resource& resource, State before, State after)
{
    META_FUNCTION_TASK();
    return Has(Barrier::Type::Transition, resource, before, after);
}

bool ResourceBase::Barriers::Add(Barrier::Type type, const Resource& resource, State before, State after)
{
    META_FUNCTION_TASK();
    return AddStateChange(Barrier::Id(type, resource), Barrier::StateChange(before, after));
}

bool ResourceBase::Barriers::AddTransition(const Resource& resource, State before, State after)
{
    META_FUNCTION_TASK();
    return AddStateChange(Barrier::Id(Barrier::Type::Transition, resource), Barrier::StateChange(before, after));
}

bool ResourceBase::Barriers::AddStateChange(const Barrier::Id& id, const Barrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    const auto [ barrier_id_and_state_change_it, barrier_added ] = m_barriers_map.try_emplace(id, state_change);
    if (barrier_added)
        return true;
    else if (barrier_id_and_state_change_it->second == state_change)
        return false;

    barrier_id_and_state_change_it->second = state_change;
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

Resource::Location::Location(const Ptr<Resource>& resource_ptr, const SubResource::Index& subresource_index, Data::Size offset)
    : m_resource_ptr(resource_ptr)
    , m_subresource_index(subresource_index)
    , m_offset(offset)
{
}

Resource& Resource::Location::GetResource() const
{
    META_CHECK_ARG_NOT_NULL_DESCR(m_resource_ptr, "can not get resource from uninitialized resource location");
    return *m_resource_ptr;
}

Resource::Descriptor::Descriptor(DescriptorHeap& in_heap, Data::Index in_index)
    : heap(in_heap)
    , index(in_index)
{
    META_FUNCTION_TASK();
}
    
bool Resource::Location::operator==(const Location& other) const noexcept
{
    return std::tie(m_resource_ptr, m_subresource_index, m_offset) ==
           std::tie(other.m_resource_ptr, other.m_subresource_index, other.m_offset);
}

Resource::SubResource::SubResource(Data::Bytes&& data, const Index& index, BytesRangeOpt data_range) noexcept
    : Data::Chunk(std::move(data))
    , m_index(index)
    , m_data_range(std::move(data_range))
{
    META_FUNCTION_TASK();
}

Resource::SubResource::SubResource(Data::ConstRawPtr p_data, Data::Size size, const Index& index, BytesRangeOpt data_range) noexcept
    : Data::Chunk(p_data, size)
    , m_index(index)
    , m_data_range(std::move(data_range))
{
    META_FUNCTION_TASK();
}

Resource::SubResource::Count::Count(Data::Size depth, Data::Size array_size, Data::Size mip_levels_count)
    : m_depth(depth)
    , m_array_size(array_size)
    , m_mip_levels_count(mip_levels_count)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_ZERO_DESCR(depth, "subresource count can not be zero");
    META_CHECK_ARG_NOT_ZERO_DESCR(array_size, "subresource count can not be zero");
    META_CHECK_ARG_NOT_ZERO_DESCR(mip_levels_count, "subresource count can not be zero");
}

void Resource::SubResource::Count::operator+=(const Index& other) noexcept
{
    META_FUNCTION_TASK();
    m_depth            = std::max(m_depth,            other.GetDepthSlice() + 1U);
    m_array_size       = std::max(m_array_size,       other.GetArrayIndex() + 1U);
    m_mip_levels_count = std::max(m_mip_levels_count, other.GetMipLevel()   + 1U);
}

bool Resource::SubResource::Count::operator==(const Count& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_depth, m_array_size, m_mip_levels_count) ==
           std::tie(other.m_depth, other.m_array_size, other.m_mip_levels_count);
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
    return fmt::format("count(d:{}, a:{}, m:{})", m_depth, m_array_size, m_mip_levels_count);
}

Resource::SubResource::Index::Index(Data::Index depth_slice, Data::Index array_index, Data::Index mip_level) noexcept
    : m_depth_slice(depth_slice)
    , m_array_index(array_index)
    , m_mip_level(mip_level)
{
    META_FUNCTION_TASK();
}

Resource::SubResource::Index::Index(Data::Index raw_index, const Count& count)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(raw_index, count.GetRawCount());

    const uint32_t array_and_depth_index = raw_index / count.GetMipLevelsCount();
    m_depth_slice = array_and_depth_index % count.GetDepth();
    m_array_index = array_and_depth_index / count.GetDepth();
    m_mip_level   = raw_index % count.GetMipLevelsCount();
}

Resource::SubResource::Index::Index(const Resource::SubResource::Count& count)
    : m_depth_slice(count.GetDepth())
    , m_array_index(count.GetArraySize())
    , m_mip_level(count.GetMipLevelsCount())
{
    META_FUNCTION_TASK();
}

bool Resource::SubResource::Index::operator==(const Index& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_depth_slice, m_array_index, m_mip_level) ==
           std::tie(other.m_depth_slice, other.m_array_index, other.m_mip_level);
}

bool Resource::SubResource::Index::operator<(const Index& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_depth_slice, m_array_index, m_mip_level) <
           std::tie(other.m_depth_slice, other.m_array_index, other.m_mip_level);
}

bool Resource::SubResource::Index::operator>=(const Index& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator<(other);
}

bool Resource::SubResource::Index::operator<(const Count& other) const noexcept
{
    META_FUNCTION_TASK();
    return m_depth_slice < other.GetDepth() &&
           m_array_index < other.GetArraySize() &&
           m_mip_level   < other.GetMipLevelsCount();
}

bool Resource::SubResource::Index::operator>=(const Count& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator<(other);
}

Resource::SubResource::Index::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    return fmt::format("index(d:{}, a:{}, m:{})", m_depth_slice, m_array_index, m_mip_level);
}

ResourceBase::ResourceBase(Type type, Usage usage_mask, ContextBase& context, const DescriptorByUsage& descriptor_by_usage)
    : m_type(type)
    , m_usage_mask(usage_mask)
    , m_context(context)
    , m_descriptor_by_usage(descriptor_by_usage)
{
    META_FUNCTION_TASK();

    for (const auto& [usage, descriptor] : m_descriptor_by_usage)
    {
        descriptor.heap.ReplaceResource(*this, descriptor.index);
    }
}

ResourceBase::~ResourceBase()
{
    META_FUNCTION_TASK();

    for (const auto& [usage, descriptor] : m_descriptor_by_usage)
    {
        descriptor.heap.RemoveResource(descriptor.index);
    }
}

void ResourceBase::InitializeDefaultDescriptors()
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;
    for (Usage usage : GetPrimaryUsageValues())
    {
        if (!magic_enum::flags::enum_contains(usage & m_usage_mask))
            continue;

        auto descriptor_by_usage_it = m_descriptor_by_usage.find(usage);
        if (descriptor_by_usage_it == m_descriptor_by_usage.end())
        {
            // Create default resource descriptor by usage
            const DescriptorHeap::Type heap_type = GetDescriptorHeapTypeByUsage(usage);
            DescriptorHeap& heap = m_context.GetResourceManager().GetDescriptorHeap(heap_type);
            m_descriptor_by_usage.try_emplace(usage, Descriptor(heap, heap.AddResource(*this)));
        }
    }
}

const Resource::Descriptor& ResourceBase::GetDescriptor(Usage usage) const
{
    META_FUNCTION_TASK();
    auto descriptor_by_usage_it = m_descriptor_by_usage.find(usage);
    META_CHECK_ARG_DESCR(usage, descriptor_by_usage_it != m_descriptor_by_usage.end(),
                         "resource '{}' does not support '{}' usage",
                         GetName(), magic_enum::flags::enum_name(usage));
    return descriptor_by_usage_it->second;
}

void ResourceBase::SetData(const SubResources& sub_resources)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_EMPTY_DESCR(sub_resources, "can not set buffer data from empty sub-resources");

    Data::Size sub_resources_data_size = 0U;
    for(const SubResource& sub_resource : sub_resources)
    {
        META_CHECK_ARG_NAME_DESCR("sub_resource", !sub_resource.IsEmptyOrNull(), "can not set empty subresource data to buffer");
        sub_resources_data_size += sub_resource.GetDataSize();

        if (m_sub_resource_count_constant)
        {
            META_CHECK_ARG_LESS(sub_resource.GetIndex(), m_sub_resource_count);
        }
        else
        {
            m_sub_resource_count += sub_resource.GetIndex();
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
    META_CHECK_ARG_LESS(sub_resource_index, m_sub_resource_count);
    return m_sub_resource_sizes[sub_resource_index.GetRawIndex(m_sub_resource_count)];
}

DescriptorHeap::Type ResourceBase::GetDescriptorHeapTypeByUsage(ResourceBase::Usage resource_usage) const
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

const Resource::Descriptor& ResourceBase::GetDescriptorByUsage(Usage usage) const
{
    META_FUNCTION_TASK();
    auto descriptor_by_usage_it = m_descriptor_by_usage.find(usage);
    META_CHECK_ARG_DESCR(usage, descriptor_by_usage_it != m_descriptor_by_usage.end(),
                         "Resource '{}' does not have descriptor for usage '{}'",
                         GetName(), magic_enum::flags::enum_name(usage));
    return descriptor_by_usage_it->second;
}

DescriptorHeap::Types ResourceBase::GetUsedDescriptorHeapTypes() const noexcept
{
    META_FUNCTION_TASK();
    DescriptorHeap::Types heap_types;
    for (const auto& [usage, descriptor] : m_descriptor_by_usage)
    {
        heap_types.insert(descriptor.heap.GetSettings().type);
    }
    return heap_types;
}

bool ResourceBase::SetState(State state, Ptr<Barriers>& out_barriers)
{
    META_FUNCTION_TASK();
    if (m_state == state)
        return false;

    META_LOG("Resource '{}' state changed from {} to {}", GetName(), GetStateName(m_state), GetStateName(state));

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
    ValidateSubResource(sub_resource.GetIndex(), sub_resource.GetDataRangeOptional());

    const Data::Index sub_resource_raw_index = sub_resource.GetIndex().GetRawIndex(m_sub_resource_count);
    const Data::Size sub_resource_data_size = m_sub_resource_sizes[sub_resource_raw_index];
    META_UNUSED(sub_resource_data_size);

    if (sub_resource.HasDataRange())
    {
        META_CHECK_ARG_EQUAL_DESCR(sub_resource.GetDataSize(), sub_resource.GetDataRange().GetLength(),
                                   "sub-resource {} data size should be equal to the length of data range", sub_resource.GetIndex());
        META_CHECK_ARG_LESS_DESCR(sub_resource.GetDataSize(), sub_resource_data_size + 1,
                                  "sub-resource {} data size should be less or equal than full resource size", sub_resource.GetIndex());
    }
    else
    {
        META_CHECK_ARG_EQUAL_DESCR(sub_resource.GetDataSize(), sub_resource_data_size,
                                   "Sub-resource {} data size should be equal to full resource size when data range is not specified", sub_resource.GetIndex());
    }
}

void ResourceBase::ValidateSubResource(const SubResource::Index& sub_resource_index, const std::optional<BytesRange>& sub_resource_data_range) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(sub_resource_index, m_sub_resource_count);
    if (!sub_resource_data_range)
        return;

    META_CHECK_ARG_NAME_DESCR("sub_resource_data_range", !sub_resource_data_range->IsEmpty(), "sub-resource {} data range can not be empty", sub_resource_index);
    const Data::Index sub_resource_raw_index = sub_resource_index.GetRawIndex(m_sub_resource_count);
    META_CHECK_ARG_LESS(sub_resource_raw_index, m_sub_resource_sizes.size());

    const Data::Size sub_resource_data_size = m_sub_resource_sizes[sub_resource_raw_index];
    META_UNUSED(sub_resource_data_size);
    META_CHECK_ARG_LESS_DESCR(sub_resource_data_range->GetEnd(), sub_resource_data_size + 1, "sub-resource index {}", sub_resource_index);
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
    const auto       prev_subresource_raw_count = static_cast<Data::Size>(m_sub_resource_sizes.size());
    if (curr_subresource_raw_count == prev_subresource_raw_count)
        return;

    m_sub_resource_sizes.reserve(curr_subresource_raw_count);
    for (Data::Index subresource_raw_index = prev_subresource_raw_count; subresource_raw_index < curr_subresource_raw_count; ++subresource_raw_index)
    {
        const SubResource::Index subresource_index(subresource_raw_index, m_sub_resource_count);
        m_sub_resource_sizes.emplace_back(CalculateSubResourceDataSize(subresource_index));
    }
}

const std::vector<Resource::Usage>& ResourceBase::GetPrimaryUsageValues() noexcept
{
    META_FUNCTION_TASK();
    static std::vector<Resource::Usage> s_primary_usages;
    if (!s_primary_usages.empty())
        return s_primary_usages;

    for (Usage usage : magic_enum::enum_values<Usage>())
    {
        using namespace magic_enum::bitwise_operators;
        if (!magic_enum::flags::enum_contains(usage & s_secondary_usage_mask) && usage != Usage::None)
            s_primary_usages.push_back(usage);
    }
    return s_primary_usages;
}

} // namespace Methane::Graphics
