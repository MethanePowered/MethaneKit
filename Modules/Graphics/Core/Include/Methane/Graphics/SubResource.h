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

FILE: Methane/Graphics/SubResource.h
Methane sub-resource used for resource data transfers.

******************************************************************************/

#pragma once

#include <Methane/Data/Chunk.hpp>
#include <Methane/Data/Range.hpp>

#include <optional>

namespace Methane::Graphics
{

using BytesRange = Data::Range<Data::Index>;
using BytesRangeOpt = std::optional<BytesRange>;

class SubResource : public Data::Chunk
{
public:
    class Index;

    class Count
    {
    public:
        Count() = default;
        explicit Count(Data::Size depth, Data::Size array_size = 1U, Data::Size mip_levels_count = 1U);

        [[nodiscard]] Data::Size GetDepth() const noexcept
        { return m_depth; }

        [[nodiscard]] Data::Size GetArraySize() const noexcept
        { return m_array_size; }

        [[nodiscard]] Data::Size GetMipLevelsCount() const noexcept
        { return m_mip_levels_count; }

        [[nodiscard]] Data::Size GetRawCount() const noexcept
        { return m_depth * m_array_size * m_mip_levels_count; }

        void operator+=(const Index& other) noexcept;
        [[nodiscard]] bool operator==(const Count& other) const noexcept;

        [[nodiscard]] bool operator!=(const Count& other) const noexcept
        { return !operator==(other); }

        [[nodiscard]] bool operator<(const Count& other) const noexcept;
        [[nodiscard]] bool operator>=(const Count& other) const noexcept;
        [[nodiscard]] explicit operator Index() const noexcept;
        [[nodiscard]] explicit operator std::string() const noexcept;

    private:
        Data::Size m_depth            = 1U;
        Data::Size m_array_size       = 1U;
        Data::Size m_mip_levels_count = 1U;
    };

    class Index
    {
    public:
        Index() = default;
        explicit Index(Data::Index depth_slice, Data::Index array_index = 0U, Data::Index mip_level = 0U) noexcept;
        Index(Data::Index raw_index, const Count& count);
        explicit Index(const Count& count);
        Index(const Index&) noexcept = default;

        Index& operator=(const Index&) noexcept = default;

        [[nodiscard]] Data::Index GetDepthSlice() const noexcept { return m_depth_slice; }
        [[nodiscard]] Data::Index GetArrayIndex() const noexcept { return m_array_index; }
        [[nodiscard]] Data::Index GetMipLevel() const noexcept   { return m_mip_level; }

        [[nodiscard]] Data::Index GetRawIndex(const Count& count) const noexcept
        { return (m_array_index * count.GetDepth() + m_depth_slice) * count.GetMipLevelsCount() + m_mip_level; }

        [[nodiscard]] bool operator==(const Index& other) const noexcept;
        [[nodiscard]] bool operator!=(const Index& other) const noexcept { return !operator==(other); }

        [[nodiscard]] bool operator<(const Index& other) const noexcept;
        [[nodiscard]] bool operator>=(const Index& other) const noexcept;
        [[nodiscard]] bool operator<(const Count& other) const noexcept;
        [[nodiscard]] bool operator>=(const Count& other) const noexcept;
        [[nodiscard]] explicit operator std::string() const noexcept;

    private:
        Data::Index m_depth_slice = 0U;
        Data::Index m_array_index = 0U;
        Data::Index m_mip_level   = 0U;
    };

    SubResource() = default;
    explicit SubResource(Data::Bytes&& data, const Index& index = Index(), BytesRangeOpt data_range = {}) noexcept;
    SubResource(Data::ConstRawPtr p_data, Data::Size size, const Index& index = Index(), BytesRangeOpt data_range = {}) noexcept;
    ~SubResource() = default;

    [[nodiscard]] const Index& GetIndex() const noexcept
    { return m_index; }

    [[nodiscard]] bool HasDataRange() const noexcept
    { return m_data_range.has_value(); }

    [[nodiscard]] const BytesRange& GetDataRange() const
    { return m_data_range.value(); }

    [[nodiscard]] const BytesRangeOpt& GetDataRangeOptional() const noexcept
    { return m_data_range; }

private:
    Index         m_index;
    BytesRangeOpt m_data_range;
};

using SubResources = std::vector<SubResource>;

struct Resource;

class ResourceLocation
{
public:
    ResourceLocation(Resource& resource, Data::Size offset = 0U);
    ResourceLocation(Resource& resource,
                     const SubResource::Index& subresource_index,
                     const SubResource::Count& subresource_count = {},
                     Data::Size offset = 0U);

    [[nodiscard]] bool operator==(const ResourceLocation& other) const noexcept;
    [[nodiscard]] bool operator!=(const ResourceLocation& other) const noexcept;
    [[nodiscard]] explicit operator std::string() const;

    [[nodiscard]] const Ptr<Resource>&      GetResourcePtr() const noexcept      { return m_resource_ptr; }
    [[nodiscard]] Resource&                 GetResource() const noexcept         { return *m_resource_ptr; }
    [[nodiscard]] const SubResource::Index& GetSubresourceIndex() const noexcept { return m_subresource_index; }
    [[nodiscard]] const SubResource::Count& GetSubresourceCount() const noexcept { return m_subresource_count; }
    [[nodiscard]] Data::Size                GetOffset() const noexcept           { return m_offset; }

private:
    Ptr<Resource>      m_resource_ptr;
    SubResource::Index m_subresource_index;
    SubResource::Count m_subresource_count;
    Data::Size         m_offset = 0U;
};

using ResourceLocations = std::vector<ResourceLocation>;

template<typename TResource>
static ResourceLocations CreateResourceLocations(const Ptrs<TResource>& resources)
{
    ResourceLocations resource_locations;
    std::transform(resources.begin(), resources.end(), std::back_inserter(resource_locations),
                   [](const Ptr<TResource>& resource_ptr) { META_CHECK_ARG_NOT_NULL(resource_ptr); return ResourceLocation(*resource_ptr); });
    return resource_locations;
}

} // namespace Methane::Graphics
