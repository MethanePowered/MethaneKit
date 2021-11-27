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

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>

#include <utility>
#include <algorithm>

namespace Methane::Graphics
{

Resource::Descriptor::Descriptor(DescriptorHeapDX& in_heap, Data::Index in_index)
    : heap(in_heap)
    , index(in_index)
{
    META_FUNCTION_TASK();
}

Resource::AllocationError::AllocationError(const Resource& resource, std::string_view error_message)
    : std::runtime_error(fmt::format("Failed to allocate memory for GPU resource '{}': {}", resource.GetName(), error_message))
    , m_resource(resource)
{
    META_FUNCTION_TASK();
}

ResourceBase::ResourceBase(Type type, Usage usage_mask, const ContextBase& context)
    : m_type(type)
    , m_usage_mask(usage_mask)
    , m_context(context)
{
    META_FUNCTION_TASK();
}

void ResourceBase::SetData(const SubResources& sub_resources, CommandQueue*)
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

const Context& ResourceBase::GetContext() const noexcept
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

bool ResourceBase::SetState(State state, Ptr<Barriers>& out_barriers)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_state_mutex);
    if (m_state == state)
    {
        if (out_barriers)
            out_barriers->RemoveTransition(*this);
        return false;
    }

    META_LOG("Resource '{}' state changed from {} to {}", GetName(), magic_enum::enum_name(m_state), magic_enum::enum_name(state));

    if (m_state != State::Common)
    {
        if (!out_barriers)
            out_barriers = Barriers::Create();
        out_barriers->AddTransition(*this, m_state, state);
    }

    m_state = state;
    return true;
}

bool ResourceBase::SetState(State state)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_state_mutex);
    if (m_state == state)
        return false;

    META_LOG("Resource '{}' state changed from {} to {}", GetName(), magic_enum::enum_name(m_state), magic_enum::enum_name(state));
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
    }
    META_CHECK_ARG_LESS_OR_EQUAL_DESCR(sub_resource.GetDataSize(), sub_resource_data_size,
                                       "sub-resource {} data size should be less or equal than full resource size", sub_resource.GetIndex());
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
