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

FILE: Methane/Graphics/Resource.h
Methane resource interface: base class of all GPU resources.

******************************************************************************/

#pragma once

#include "Object.h"

#include <Methane/Memory.hpp>
#include <Methane/Data/Chunk.hpp>
#include <Methane/Data/Range.hpp>
#include <Methane/Graphics/Types.h>

#include <fmt/format.h>

#include <string>
#include <vector>
#include <map>
#include <array>
#include <optional>

namespace Methane::Graphics
{

struct Context;
class DescriptorHeap;

struct Resource : virtual Object
{
    enum class Type : uint32_t
    {
        Buffer = 0U,
        Texture,
        Sampler,
    };

    struct Usage
    {
        using Mask = uint32_t;
        enum Value : Mask
        {
            Unknown      = 0U,
            // Primary usages
            ShaderRead   = 1U << 0U,
            ShaderWrite  = 1U << 1U,
            RenderTarget = 1U << 2U,
            ReadBack     = 1U << 3U,
            // Secondary usages
            Addressable  = 1U << 4U,
            All          = ~0U,
        };

        using BaseValues = std::array<Value, 4>;
        static constexpr const BaseValues primary_values{ ShaderRead, ShaderWrite, RenderTarget };

        using Values = std::array<Value, 5>;
        static constexpr const Values values{ ShaderRead, ShaderWrite, RenderTarget, ReadBack, Addressable };

        static std::string ToString(Usage::Value usage);
        static std::string ToString(Usage::Mask usage_mask);

        Usage() = delete;
    };

    struct Descriptor
    {
        DescriptorHeap& heap;
        Data::Index     index;

        Descriptor(DescriptorHeap& in_heap, Data::Index in_index);
    };

    using DescriptorByUsage = std::map<Usage::Value, Descriptor>;
    
    class Location
    {
    public:
        Location(Ptr<Resource> resource_ptr, Data::Size offset = 0U);

        bool operator==(const Location& other) const noexcept;

        const Ptr<Resource>& GetResourcePtr() const noexcept  { return m_resource_ptr; }
        Resource&            GetResource() const noexcept     { return *m_resource_ptr; }
        Data::Size           GetOffset() const noexcept       { return m_offset; }

    private:
        Ptr<Resource> m_resource_ptr;
        Data::Size    m_offset;
    };

    using Locations = std::vector<Location>;

    template<typename TResource>
    static Locations CreateLocations(const std::vector<std::shared_ptr<TResource>>& resources)
    {
        Resource::Locations resource_locations;
        std::transform(resources.begin(), resources.end(), std::back_inserter(resource_locations),
                       [](const std::shared_ptr<TResource>& resource_ptr) { return Location(resource_ptr); });
        return resource_locations;
    }

    using BytesRange = Data::Range<Data::Index>;

    class SubResource : public Data::Chunk
    {
    public:
        class Index;

        class Count
        {
        public:
            explicit Count(Data::Size array_size  = 1U, Data::Size depth  = 1U, Data::Size mip_levels_count = 1U);
            Count(const Count&) noexcept = default;

            Count& operator=(const Count&) noexcept = default;

            Data::Size GetDepth() const noexcept          { return m_depth; }
            Data::Size GetArraySize() const noexcept      { return m_array_size; }
            Data::Size GetMipLevelsCount() const noexcept { return m_mip_levels_count; }
            Data::Size GetRawCount() const noexcept       { return m_depth * m_array_size * m_mip_levels_count; }

            void operator+=(const Index& other) noexcept;
            bool operator==(const Count& other) const noexcept;
            bool operator!=(const Count& other) const noexcept { return !operator==(other); }
            bool operator<(const Count& other) const noexcept;
            bool operator>=(const Count& other) const noexcept;
            explicit operator Index() const noexcept;
            explicit operator std::string() const noexcept;

        private:
            Data::Size m_depth;
            Data::Size m_array_size;
            Data::Size m_mip_levels_count;
        };

        class Index
        {
        public:
            explicit Index(Data::Index depth_slice  = 0U, Data::Index array_index  = 0U, Data::Index mip_level = 0U) noexcept;
            Index(Data::Index raw_index, const Count& count);
            explicit Index(const Count& count);
            Index(const Index&) noexcept = default;

            Index& operator=(const Index&) noexcept = default;

            Data::Index GetDepthSlice() const noexcept { return m_depth_slice; }
            Data::Index GetArrayIndex() const noexcept { return m_array_index; }
            Data::Index GetMipLevel() const noexcept   { return m_mip_level; }
            Data::Index GetRawIndex(const Count& count) const noexcept
            { return (m_array_index * count.GetDepth() + m_depth_slice) * count.GetMipLevelsCount() + m_mip_level; }

            bool operator==(const Index& other) const noexcept;
            bool operator!=(const Index& other) const noexcept { return !operator==(other); }
            bool operator<(const Index& other) const noexcept;
            bool operator>=(const Index& other) const noexcept;
            bool operator<(const Count& other) const noexcept;
            bool operator>=(const Count& other) const noexcept;
            explicit operator std::string() const noexcept;

        private:
            Data::Index m_depth_slice;
            Data::Index m_array_index;
            Data::Index m_mip_level;
        };

        using BytesRangeOpt = std::optional<BytesRange>;

        explicit SubResource(SubResource&& other) noexcept;
        explicit SubResource(const SubResource& other) noexcept;
        explicit SubResource(Data::Bytes&& data, const Index& index = Index(), BytesRangeOpt data_range = {}) noexcept;
        SubResource(Data::ConstRawPtr p_data, Data::Size size, const Index& index = Index(), BytesRangeOpt data_range = {}) noexcept;
        ~SubResource() = default;

        const Index&         GetIndex() const noexcept             { return m_index; }
        bool                 HasDataRange() const noexcept         { return m_data_range.has_value(); }
        const BytesRange&    GetDataRange() const                  { return m_data_range.value(); }
        const BytesRangeOpt& GetDataRangeOptional() const noexcept { return m_data_range; }

    private:
        Index         m_index;
        BytesRangeOpt m_data_range;
    };

    using SubResources = std::vector<SubResource>;

    // Auxiliary functions
    static std::string GetTypeName(Type type);

    // Resource interface
    virtual void                      SetData(const SubResources& sub_resources) = 0;
    virtual SubResource               GetData(const SubResource::Index& sub_resource_index = SubResource::Index(), const std::optional<BytesRange>& data_range = {}) = 0;
    virtual Data::Size                GetDataSize(Data::MemoryState size_type = Data::MemoryState::Reserved) const noexcept = 0;
    virtual Data::Size                GetSubResourceDataSize(const SubResource::Index& sub_resource_index = SubResource::Index()) const = 0;
    virtual const SubResource::Count& GetSubresourceCount() const noexcept = 0;
    virtual Type                      GetResourceType() const noexcept = 0;
    virtual Usage::Mask               GetUsageMask() const noexcept = 0;
    virtual const DescriptorByUsage&  GetDescriptorByUsage() const noexcept = 0;
    virtual const Descriptor&         GetDescriptor(Usage::Value usage) const = 0;
    virtual Context&                  GetContext() noexcept = 0;
};

} // namespace Methane::Graphics

