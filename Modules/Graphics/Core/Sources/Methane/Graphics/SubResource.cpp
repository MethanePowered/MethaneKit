/******************************************************************************

Copyright 2020-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/SubResource.cpp
Methane sub-resource used for resource data transfers.

******************************************************************************/

#include "CoreFormatters.hpp"

#include <Methane/Graphics/SubResource.h>
#include <Methane/Graphics/Resource.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <fmt/format.h>
#include <magic_enum.hpp>

namespace Methane::Graphics
{

SubResource::SubResource(Data::Bytes&& data, const Index& index, BytesRangeOpt data_range) noexcept
    : Data::Chunk(std::move(data))
    , m_index(index)
    , m_data_range(std::move(data_range))
{
    META_FUNCTION_TASK();
}

SubResource::SubResource(Data::ConstRawPtr p_data, Data::Size size, const Index& index, BytesRangeOpt data_range) noexcept
    : Data::Chunk(p_data, size)
    , m_index(index)
    , m_data_range(std::move(data_range))
{
    META_FUNCTION_TASK();
}

SubResource::Count::Count(Data::Size depth, Data::Size array_size, Data::Size mip_levels_count)
    : m_depth(depth)
    , m_array_size(array_size)
    , m_mip_levels_count(mip_levels_count)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_ZERO_DESCR(depth, "subresource count can not be zero");
    META_CHECK_ARG_NOT_ZERO_DESCR(array_size, "subresource count can not be zero");
    META_CHECK_ARG_NOT_ZERO_DESCR(mip_levels_count, "subresource count can not be zero");
}

void SubResource::Count::operator+=(const Index& other) noexcept
{
    META_FUNCTION_TASK();
    m_depth            = std::max(m_depth,            other.GetDepthSlice() + 1U);
    m_array_size       = std::max(m_array_size,       other.GetArrayIndex() + 1U);
    m_mip_levels_count = std::max(m_mip_levels_count, other.GetMipLevel()   + 1U);
}

bool SubResource::Count::operator==(const Count& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_depth, m_array_size, m_mip_levels_count) ==
           std::tie(other.m_depth, other.m_array_size, other.m_mip_levels_count);
}

bool SubResource::Count::operator<(const Count& other) const noexcept
{
    META_FUNCTION_TASK();
    return GetRawCount() < other.GetRawCount();
}

bool SubResource::Count::operator>=(const Count& other) const noexcept
{
    META_FUNCTION_TASK();
    return GetRawCount() >= other.GetRawCount();
}

SubResource::Count::operator SubResource::Index() const noexcept
{
    META_FUNCTION_TASK();
    return SubResource::Index(*this);
}

SubResource::Count::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    return fmt::format("count(d:{}, a:{}, m:{})", m_depth, m_array_size, m_mip_levels_count);
}

SubResource::Index::Index(Data::Index depth_slice, Data::Index array_index, Data::Index mip_level) noexcept
    : m_depth_slice(depth_slice)
    , m_array_index(array_index)
    , m_mip_level(mip_level)
{
    META_FUNCTION_TASK();
}

SubResource::Index::Index(Data::Index raw_index, const Count& count)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(raw_index, count.GetRawCount());

    const uint32_t array_and_depth_index = raw_index / count.GetMipLevelsCount();
    m_depth_slice = array_and_depth_index % count.GetDepth();
    m_array_index = array_and_depth_index / count.GetDepth();
    m_mip_level   = raw_index % count.GetMipLevelsCount();
}

SubResource::Index::Index(const SubResource::Count& count)
    : m_depth_slice(count.GetDepth())
    , m_array_index(count.GetArraySize())
    , m_mip_level(count.GetMipLevelsCount())
{
    META_FUNCTION_TASK();
}

bool SubResource::Index::operator==(const Index& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_depth_slice, m_array_index, m_mip_level) ==
           std::tie(other.m_depth_slice, other.m_array_index, other.m_mip_level);
}

bool SubResource::Index::operator<(const Index& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_depth_slice, m_array_index, m_mip_level) <
           std::tie(other.m_depth_slice, other.m_array_index, other.m_mip_level);
}

bool SubResource::Index::operator>=(const Index& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator<(other);
}

bool SubResource::Index::operator<(const Count& other) const noexcept
{
    META_FUNCTION_TASK();
    return m_depth_slice < other.GetDepth() &&
           m_array_index < other.GetArraySize() &&
           m_mip_level   < other.GetMipLevelsCount();
}

bool SubResource::Index::operator>=(const Count& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator<(other);
}

SubResource::Index::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    return fmt::format("index(d:{}, a:{}, m:{})", m_depth_slice, m_array_index, m_mip_level);
}

ResourceLocation::ResourceLocation(const Ptr<Resource>& resource_ptr, const SubResource::Index& subresource_index, Data::Size offset)
    : m_resource_ptr(resource_ptr)
    , m_subresource_index(subresource_index)
    , m_offset(offset)
{
    META_FUNCTION_TASK();
}

Resource& ResourceLocation::GetResource() const
{
    META_CHECK_ARG_NOT_NULL_DESCR(m_resource_ptr, "can not get resource from uninitialized resource location");
    return *m_resource_ptr;
}

bool ResourceLocation::operator==(const ResourceLocation& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_resource_ptr, m_subresource_index, m_offset) ==
           std::tie(other.m_resource_ptr, other.m_subresource_index, other.m_offset);
}

bool ResourceLocation::operator!=(const ResourceLocation& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_resource_ptr, m_subresource_index, m_offset) !=
           std::tie(other.m_resource_ptr, other.m_subresource_index, other.m_offset);
}

ResourceLocation::operator std::string() const
{
    META_FUNCTION_TASK();
    if (!m_resource_ptr)
        return "Null resource location";

    return fmt::format("{} '{}' subresource {} with offset {}",
                       magic_enum::enum_name(m_resource_ptr->GetResourceType()),
                       m_resource_ptr->GetName(), m_subresource_index, m_offset);
}

} // namespace Methane::Graphics
