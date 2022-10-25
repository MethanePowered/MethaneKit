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

FILE: Methane/Graphics/ResourceView.h
Methane sub-resource used for resource data transfers
and resource view used in program bindings.

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
        Count() noexcept = default;
        explicit Count(Data::Size depth, Data::Size array_size = 1U, Data::Size mip_levels_count = 1U);

        [[nodiscard]] Data::Size GetDepth() const noexcept
        { return m_depth; }

        [[nodiscard]] Data::Size GetArraySize() const noexcept
        { return m_array_size; }

        [[nodiscard]] Data::Size GetMipLevelsCount() const noexcept
        { return m_mip_levels_count; }

        [[nodiscard]] Data::Size GetRawCount() const noexcept
        { return m_array_size * m_depth * m_mip_levels_count; }

        [[nodiscard]] Data::Index GetBaseLayerCount() const noexcept
        { return m_array_size * m_depth; }

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
        Index() noexcept { } // NOSONAR = default does not work for Clang here, but works fine for Count(), which is weird.
        explicit Index(Data::Index depth_slice, Data::Index array_index = 0U, Data::Index mip_level = 0U) noexcept;
        Index(Data::Index raw_index, const Count& count);
        explicit Index(const Count& count);
        Index(const Index&) noexcept = default;

        Index& operator=(const Index&) noexcept = default;

        [[nodiscard]] Data::Index GetDepthSlice() const noexcept { return m_depth_slice; }
        [[nodiscard]] Data::Index GetArrayIndex() const noexcept { return m_array_index; }
        [[nodiscard]] Data::Index GetMipLevel() const noexcept   { return m_mip_level; }

        [[nodiscard]] Data::Index GetRawIndex(const Count& count) const noexcept
        { return GetBaseLayerIndex(count) * count.GetMipLevelsCount() + m_mip_level; }

        [[nodiscard]] Data::Index GetBaseLayerIndex(const Count& count) const noexcept
        { return (m_array_index * count.GetDepth() + m_depth_slice); }

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
    explicit SubResource(Data::Bytes&& data, const Index& index = {}, BytesRangeOpt data_range = {}) noexcept;
    SubResource(Data::ConstRawPtr p_data, Data::Size size, const Index& index = {}, BytesRangeOpt data_range = {}) noexcept;
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

struct IResource;

enum class ResourceUsage : uint32_t
{
    None         = 0U,
    // Primary usages
    ShaderRead   = 1U << 0U,
    ShaderWrite  = 1U << 1U,
    RenderTarget = 1U << 2U,
    // Secondary usages
    ReadBack     = 1U << 3U,
    Addressable  = 1U << 4U,
};

enum class TextureDimensionType : uint32_t
{
    Tex1D = 0,
    Tex1DArray,
    Tex2D,
    Tex2DArray,
    Tex2DMultisample,
    Cube,
    CubeArray,
    Tex3D,
};

class ResourceView
{
public:
    struct Settings
    {
        SubResource::Index        subresource_index;
        SubResource::Count        subresource_count;
        Data::Size                offset = 0U;
        Data::Size                size   = 0U;
        Opt<TextureDimensionType> texture_dimension_type_opt;

        [[nodiscard]] bool operator<(const Settings& other) const noexcept;
        [[nodiscard]] bool operator==(const Settings& other) const noexcept;
        [[nodiscard]] bool operator!=(const Settings& other) const noexcept;
    };

    struct Id : Settings
    {
        ResourceUsage usage;
        Id(ResourceUsage usage, const Settings& settings);

        [[nodiscard]] bool operator<(const Id& other) const noexcept;
        [[nodiscard]] bool operator==(const Id& other) const noexcept;
        [[nodiscard]] bool operator!=(const Id& other) const noexcept;
    };

    ResourceView(IResource& resource, const Settings& settings);
    ResourceView(IResource& resource, Data::Size offset = 0U, Data::Size size = 0U);
    ResourceView(IResource& resource,
                 const SubResource::Index& subresource_index,
                 const SubResource::Count& subresource_count = {},
                 Data::Size offset = 0U,
                 Data::Size size   = 0U);
    ResourceView(IResource& resource,
                 const SubResource::Index& subresource_index,
                 const SubResource::Count& subresource_count = {},
                 Opt<TextureDimensionType> texture_dimension_type_opt = std::nullopt);

    [[nodiscard]] bool operator==(const ResourceView& other) const noexcept;
    [[nodiscard]] bool operator!=(const ResourceView& other) const noexcept;
    [[nodiscard]] explicit operator std::string() const;

    [[nodiscard]] const Ptr<IResource>&      GetResourcePtr() const noexcept      { return m_resource_ptr; }
    [[nodiscard]] IResource&                 GetResource() const noexcept         { return *m_resource_ptr; }
    [[nodiscard]] const Settings&           GetSettings() const noexcept         { return m_settings; }
    [[nodiscard]] const SubResource::Index& GetSubresourceIndex() const noexcept { return m_settings.subresource_index; }
    [[nodiscard]] const SubResource::Count& GetSubresourceCount() const noexcept { return m_settings.subresource_count; }
    [[nodiscard]] Data::Size                GetOffset() const noexcept           { return m_settings.offset; }
    [[nodiscard]] TextureDimensionType      GetTextureDimensionType() const;

private:
    Ptr<IResource> m_resource_ptr;
    Settings       m_settings;
};

using ResourceViews = std::vector<ResourceView>;

template<typename TResource>
static ResourceViews CreateResourceViews(const Ptrs<TResource>& resources)
{
    ResourceViews resource_views;
    std::transform(resources.begin(), resources.end(), std::back_inserter(resource_views),
                   [](const Ptr<TResource>& resource_ptr) { META_CHECK_ARG_NOT_NULL(resource_ptr); return ResourceView(*resource_ptr); });
    return resource_views;
}

} // namespace Methane::Graphics
