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

FILE: Methane/Graphics/Base/Resource.cpp
Base implementation of the resource interface.

******************************************************************************/

#include <Methane/Graphics/Base/Resource.h>
#include <Methane/Graphics/Base/Texture.h>
#include <Methane/Graphics/Base/Context.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <utility>
#include <algorithm>

template<>
struct fmt::formatter<Methane::Graphics::SubResource::Index>
{
    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }

    template<typename FormatContext>
    auto format(const Methane::Graphics::SubResource::Index& index, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", static_cast<std::string>(index));
    }
};

template<>
struct fmt::formatter<Methane::Graphics::SubResource::Count>
{
    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }

    template<typename FormatContext>
    auto format(const Methane::Graphics::SubResource::Count& count, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", static_cast<std::string>(count));
    }
};

namespace Methane::Graphics::Base
{

Resource::Resource(const Context& context, Type type, Usage usage_mask,
                           State initial_state, Opt<State> auto_transition_source_state_opt)
    : m_context(context)
    , m_type(type)
    , m_usage_mask(usage_mask)
    , m_state(initial_state)
    , m_auto_transition_source_state_opt(auto_transition_source_state_opt)
{
    META_FUNCTION_TASK();
}

void Resource::SetData(const SubResources& sub_resources, ICommandQueue&)
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

IResource::SubResource Resource::GetData(const SubResource::Index&, const std::optional<BytesRange>&)
{
    META_FUNCTION_NOT_IMPLEMENTED_RETURN_DESCR(IResource::SubResource(), "reading data is not allowed for this type of resource");
}

const IContext& Resource::GetContext() const noexcept
{
    META_FUNCTION_TASK();
    return m_context;
}

Data::Size Resource::GetSubResourceDataSize(const SubResource::Index& sub_resource_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(sub_resource_index, m_sub_resource_count);
    return m_sub_resource_sizes[sub_resource_index.GetRawIndex(m_sub_resource_count)];
}

bool Resource::SetState(State state, Ptr<IBarriers>& out_barriers)
{
    META_FUNCTION_TASK();
    if (!m_is_state_change_updates_barriers)
        Resource::SetState(state);

    std::scoped_lock lock_guard(m_state_mutex);
    if (m_state == state)
    {
        if (out_barriers)
            out_barriers->RemoveStateTransition(*this);

        return false;
    }

    META_LOG("{} resource '{}' state changed from {} to {} with barrier update",
             magic_enum::enum_name(GetResourceType()), GetName(),
             magic_enum::enum_name(m_state), magic_enum::enum_name(state));

    if (m_state != m_auto_transition_source_state_opt)
    {
        if (!out_barriers)
            out_barriers = IResourceBarriers::Create();

        out_barriers->AddStateTransition(*this, m_state, state);
    }

    m_state = state;
    return true;
}

bool Resource::SetState(State state)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_state_mutex);
    if (m_state == state)
        return false;

    META_LOG("{} resource '{}' state changed from {} to {}",
             magic_enum::enum_name(GetResourceType()), GetName(),
             magic_enum::enum_name(m_state), magic_enum::enum_name(state));

    m_state = state;
    return true;
}

bool Resource::SetOwnerQueueFamily(uint32_t family_index, Ptr<IBarriers>& out_barriers)
{
    META_FUNCTION_TASK();
    if (m_owner_queue_family_index_opt == family_index)
    {
        if (out_barriers)
            out_barriers->RemoveOwnerTransition(*this);
        return false;
    }

    META_LOG("{} resource '{}' owner queue changed from {} to {} queue family {} barrier update",
             magic_enum::enum_name(GetResourceType()), GetName(),
             m_owner_queue_family_index_opt ? std::to_string(*m_owner_queue_family_index_opt) : "n/a",
             family_index,
             m_owner_queue_family_index_opt ? "with" : "without");

    if (m_owner_queue_family_index_opt)
    {
        if (!out_barriers)
            out_barriers = IBarriers::Create();

        out_barriers->AddOwnerTransition(*this, *m_owner_queue_family_index_opt, family_index);
    }

    m_owner_queue_family_index_opt = family_index;
    return true;
}

bool Resource::SetOwnerQueueFamily(uint32_t family_index)
{
    META_FUNCTION_TASK();
    if (m_owner_queue_family_index_opt == family_index)
        return false;

    META_LOG("{} resource '{}' owner queue changed from {} to {} queue family",
             magic_enum::enum_name(GetResourceType()), GetName(),
             m_owner_queue_family_index_opt ? std::to_string(*m_owner_queue_family_index_opt) : "n/a",
             family_index);

    m_owner_queue_family_index_opt = family_index;
    return true;
}

void Resource::SetSubResourceCount(const SubResource::Count& sub_resource_count)
{
    META_FUNCTION_TASK();

    m_sub_resource_count_constant = true;
    m_sub_resource_count          = sub_resource_count;
    m_sub_resource_sizes.clear();

    FillSubresourceSizes();
}

void Resource::ValidateSubResource(const SubResource& sub_resource) const
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

void Resource::ValidateSubResource(const SubResource::Index& sub_resource_index, const std::optional<BytesRange>& sub_resource_data_range) const
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

Data::Size Resource::CalculateSubResourceDataSize(const SubResource::Index& subresource_index) const
{
    META_FUNCTION_TASK();
    static const SubResource::Index s_zero_index;
    META_CHECK_ARG_EQUAL_DESCR(subresource_index, s_zero_index, "subresource size is undefined, must be provided by super class override");

    static const SubResource::Count s_one_count;
    META_CHECK_ARG_EQUAL_DESCR(m_sub_resource_count, s_one_count, "subresource size is undefined, must be provided by super class override");

    return GetDataSize();
}

void Resource::FillSubresourceSizes()
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

} // namespace Methane::Graphics::Base
