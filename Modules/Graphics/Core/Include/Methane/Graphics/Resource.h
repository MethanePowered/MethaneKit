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

FILE: Methane/Graphics/Resource.h
Methane resource interface: base class of all GPU resources.

******************************************************************************/

#pragma once

#include "Object.h"
#include "Types.h"

#include <Methane/Memory.hpp>

#include <string>
#include <vector>
#include <map>
#include <array>

namespace Methane::Graphics
{

class DescriptorHeap;

struct Resource : virtual Object
{
    enum class Type : uint32_t
    {
        Buffer = 0u,
        Texture,
        Sampler,
    };

    struct Usage
    {
        using Mask = uint32_t;
        enum Value : Mask
        {
            Unknown      = 0u,
            // Primary usages
            ShaderRead   = 1u << 0u,
            ShaderWrite  = 1u << 1u,
            RenderTarget = 1u << 2u,
            // Secondary usages
            Addressable  = 1u << 3u,
            All          = ~0u,
        };

        using BaseValues = std::array<Value, 3>;
        static constexpr const BaseValues primary_values = { ShaderRead, ShaderWrite, RenderTarget };

        using Values = std::array<Value, 4>;
        static constexpr const Values values = { ShaderRead, ShaderWrite, RenderTarget, Addressable };

        static std::string ToString(Usage::Value usage) noexcept;
        static std::string ToString(Usage::Mask usage_mask) noexcept;

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
        Location(Ptr<Resource> sp_resource, Data::Size offset = 0u);

        bool operator==(const Location& other) const;

        const Ptr<Resource>& GetResourcePtr() const   { return m_sp_resource; }
        Resource&            GetResource() const      { return *m_sp_resource; }
        Data::Size           GetOffset() const        { return m_offset; }

    private:
        Ptr<Resource> m_sp_resource;
        Data::Size    m_offset;
    };

    using Locations = std::vector<Location>;

    template<typename TResource>
    static Locations CreateLocations(const std::vector<std::shared_ptr<TResource>>& resources)
    {
        Resource::Locations resource_locations;
        std::transform(resources.begin(), resources.end(), std::back_inserter(resource_locations),
                       [](const std::shared_ptr<TResource>& sp_resource) { return Location(sp_resource); });
        return resource_locations;
    }

    struct SubResource
    {
        struct Index
        {
            uint32_t depth_slice  = 0u;
            uint32_t array_index  = 0u;
            uint32_t mip_level    = 0u;
        };

        Data::Bytes         data_storage;
        Data::ConstRawPtr   p_data      = nullptr;
        Data::Size          data_size   = 0u;
        Index               index;

        SubResource() = default;
        SubResource(Data::Bytes&& data, Index in_index = { 0u, 0u, 0u });
        SubResource(Data::ConstRawPtr in_p_data, Data::Size in_data_size, Index in_index = { 0u, 0u, 0u });

        uint32_t GetRawIndex(uint32_t depth = 1u, uint32_t mip_levels_count = 1u) const
        { return ComputeRawIndex(index, depth, mip_levels_count); }

        static uint32_t ComputeRawIndex(const Index& index, uint32_t depth = 1, uint32_t mip_levels_count = 1)
        { return (index.array_index * depth + index.depth_slice) * mip_levels_count + index.mip_level; }

        static Index ComputeIndex(uint32_t raw_index, uint32_t depth = 1, uint32_t mip_levels_count = 1);
    };

    using SubResources = std::vector<SubResource>;

    // Auxiliary functions
    static std::string GetTypeName(Type type) noexcept;

    // Resource interface
    virtual void                      SetData(const SubResources& sub_resources) = 0;
    virtual Data::Size                GetDataSize() const = 0;
    virtual Type                      GetResourceType() const noexcept = 0;
    virtual Usage::Mask               GetUsageMask() const noexcept = 0;
    virtual const DescriptorByUsage&  GetDescriptorByUsage() const noexcept = 0;
    virtual const Descriptor&         GetDescriptor(Usage::Value usage) const = 0;
};

} // namespace Methane::Graphics
