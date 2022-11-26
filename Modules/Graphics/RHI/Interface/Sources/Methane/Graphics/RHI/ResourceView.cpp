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

FILE: Methane/Graphics/RHI/SubResource.cpp
Methane sub-resource used for resource data transfers.

******************************************************************************/

#include <Methane/Graphics/RHI/ResourceView.h>
#include <Methane/Graphics/RHI/IResource.h>
#include <Methane/Graphics/RHI/ITexture.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <fmt/format.h>
#include <magic_enum.hpp>

namespace Methane::Graphics::Rhi
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

SubResourceCount::SubResourceCount(Data::Size depth, Data::Size array_size, Data::Size mip_levels_count)
    : m_depth(depth)
    , m_array_size(array_size)
    , m_mip_levels_count(mip_levels_count)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_ZERO_DESCR(depth, "subresource count can not be zero");
    META_CHECK_ARG_NOT_ZERO_DESCR(array_size, "subresource count can not be zero");
    META_CHECK_ARG_NOT_ZERO_DESCR(mip_levels_count, "subresource count can not be zero");
}

void SubResourceCount::operator+=(const SubResourceIndex& other) noexcept
{
    META_FUNCTION_TASK();
    m_depth            = std::max(m_depth,            other.GetDepthSlice() + 1U);
    m_array_size       = std::max(m_array_size,       other.GetArrayIndex() + 1U);
    m_mip_levels_count = std::max(m_mip_levels_count, other.GetMipLevel()   + 1U);
}

bool SubResourceCount::operator==(const SubResourceCount& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_depth, m_array_size, m_mip_levels_count) ==
           std::tie(other.m_depth, other.m_array_size, other.m_mip_levels_count);
}

bool SubResourceCount::operator<(const SubResourceCount& other) const noexcept
{
    META_FUNCTION_TASK();
    return GetRawCount() < other.GetRawCount();
}

bool SubResourceCount::operator>=(const SubResourceCount& other) const noexcept
{
    META_FUNCTION_TASK();
    return GetRawCount() >= other.GetRawCount();
}

SubResourceCount::operator SubResourceIndex() const noexcept
{
    META_FUNCTION_TASK();
    return SubResourceIndex(*this);
}

SubResourceCount::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    return fmt::format("count(d:{}, a:{}, m:{})", m_depth, m_array_size, m_mip_levels_count);
}

SubResourceIndex::SubResourceIndex(Data::Index depth_slice, Data::Index array_index, Data::Index mip_level) noexcept
    : m_depth_slice(depth_slice)
    , m_array_index(array_index)
    , m_mip_level(mip_level)
{
    META_FUNCTION_TASK();
}

SubResourceIndex::SubResourceIndex(Data::Index raw_index, const SubResourceCount& count)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(raw_index, count.GetRawCount());

    const uint32_t array_and_depth_index = raw_index / count.GetMipLevelsCount();
    m_depth_slice = array_and_depth_index % count.GetDepth();
    m_array_index = array_and_depth_index / count.GetDepth();
    m_mip_level   = raw_index % count.GetMipLevelsCount();
}

SubResourceIndex::SubResourceIndex(const SubResourceCount& count)
    : m_depth_slice(count.GetDepth())
    , m_array_index(count.GetArraySize())
    , m_mip_level(count.GetMipLevelsCount())
{
    META_FUNCTION_TASK();
}

bool SubResourceIndex::operator==(const SubResourceIndex& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_depth_slice, m_array_index, m_mip_level) ==
           std::tie(other.m_depth_slice, other.m_array_index, other.m_mip_level);
}

bool SubResourceIndex::operator<(const SubResourceIndex& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_depth_slice, m_array_index, m_mip_level) <
           std::tie(other.m_depth_slice, other.m_array_index, other.m_mip_level);
}

bool SubResourceIndex::operator>=(const SubResourceIndex& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator<(other);
}

bool SubResourceIndex::operator<(const SubResourceCount& other) const noexcept
{
    META_FUNCTION_TASK();
    return m_depth_slice < other.GetDepth() &&
           m_array_index < other.GetArraySize() &&
           m_mip_level   < other.GetMipLevelsCount();
}

bool SubResourceIndex::operator>=(const SubResourceCount& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator<(other);
}

SubResourceIndex::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    return fmt::format("index(d:{}, a:{}, m:{})", m_depth_slice, m_array_index, m_mip_level);
}

bool ResourceViewSettings::operator<(const ResourceViewSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    // Do not include 'offset' in the less comparison, because native views are created without offset which is applied dynamically
    return std::tie(subresource_index, subresource_count, /*offset,*/ size) <
           std::tie(other.subresource_index, other.subresource_count, /*other.offset,*/ other.size);
}

bool ResourceViewSettings::operator==(const ResourceViewSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(subresource_index, subresource_count, offset, size) ==
           std::tie(other.subresource_index, other.subresource_count, other.offset, other.size);
}

bool ResourceViewSettings::operator!=(const ResourceViewSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(subresource_index, subresource_count, offset, size) !=
           std::tie(other.subresource_index, other.subresource_count, other.offset, other.size);
}

ResourceViewId::ResourceViewId(ResourceUsageMask usage, const ResourceViewSettings& settings)
    : ResourceViewSettings(settings)
    , usage(usage)
{
    META_FUNCTION_TASK();
}

bool ResourceViewId::operator<(const ResourceViewId& other) const noexcept
{
    META_FUNCTION_TASK();
    if (usage != other.usage)
        return usage < other.usage;

    return ResourceViewSettings::operator<(other);
}

bool ResourceViewId::operator==(const ResourceViewId& other) const noexcept
{
    META_FUNCTION_TASK();
    return usage == other.usage && ResourceViewSettings::operator==(other);
}

bool ResourceViewId::operator!=(const ResourceViewId& other) const noexcept
{
    META_FUNCTION_TASK();
    return usage != other.usage && ResourceViewSettings::operator!=(other);
}

ResourceView::ResourceView(IResource& resource, const Settings& settings)
    : m_resource_ptr(std::dynamic_pointer_cast<IResource>(resource.GetPtr()))
    , m_settings(settings)
{
    META_FUNCTION_TASK();
}

ResourceView::ResourceView(IResource& resource, Data::Size offset, Data::Size size)
    : ResourceView(resource, SubResourceIndex(), resource.GetSubresourceCount(), offset, size)
{
    META_FUNCTION_TASK();
}

ResourceView::ResourceView(IResource& resource,
                           const SubResourceIndex& subresource_index,
                           const SubResourceCount& subresource_count,
                           Data::Size offset,
                           Data::Size size)
    : ResourceView(resource, Settings{
        subresource_index,
        subresource_count,
        offset,
        size
    })
{
    META_FUNCTION_TASK();
}

ResourceView::ResourceView(IResource& resource,
                           const SubResourceIndex& subresource_index,
                           const SubResourceCount& subresource_count,
                           Opt<TextureDimensionType> texture_dimension_type_opt)
    : ResourceView(resource, Settings{
        subresource_index,
        subresource_count,
        0U, // offset
        0U, // size
        texture_dimension_type_opt
    })
{
    META_FUNCTION_TASK();
}

bool ResourceView::operator==(const ResourceView& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_resource_ptr, m_settings) ==
           std::tie(other.m_resource_ptr, other.m_settings);
}

bool ResourceView::operator!=(const ResourceView& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_resource_ptr, m_settings) !=
           std::tie(other.m_resource_ptr, other.m_settings);
}

ResourceView::operator std::string() const
{
    META_FUNCTION_TASK();
    if (!m_resource_ptr)
        return "Null resource view_id";

    return fmt::format("{} '{}' subresources from {} count {} with offset {}",
                       magic_enum::enum_name(m_resource_ptr->GetResourceType()),
                       m_resource_ptr->GetName(),
                       static_cast<std::string>(m_settings.subresource_index),
                       static_cast<std::string>(m_settings.subresource_count),
                       m_settings.offset);
}

TextureDimensionType ResourceView::GetTextureDimensionType() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_resource_ptr);
    META_CHECK_ARG_EQUAL(m_resource_ptr->GetResourceType(), Rhi::IResource::Type::Texture);
    return m_settings.texture_dimension_type_opt.value_or(dynamic_cast<ITexture&>(*m_resource_ptr).GetSettings().dimension_type);
}

} // namespace Methane::Graphics::Rhi
