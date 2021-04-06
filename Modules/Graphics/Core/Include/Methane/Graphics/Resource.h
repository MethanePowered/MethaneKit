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
#include <Methane/Data/IEmitter.h>
#include <Methane/Graphics/Types.h>

#include <fmt/format.h>

#include <magic_enum.hpp>
#include <string>
#include <array>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <optional>

namespace Methane::Graphics
{

struct Context;
struct CommandQueue;
struct Resource;
class DescriptorHeap;

struct IResourceCallback
{
    virtual void OnResourceReleased(const Resource& resource) = 0;

    virtual ~IResourceCallback() = default;
};

struct Resource
    : virtual Object
    , virtual Data::IEmitter<IResourceCallback>
{
    enum class Type
    {
        Buffer,
        Texture,
        Sampler,
    };

    enum class Usage : uint32_t
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

    static constexpr Usage s_secondary_usage_mask = []() constexpr
    {
        using namespace magic_enum::bitwise_operators;
        return Usage::Addressable | Usage::ReadBack;
    }();

    enum class State
    {
        Common,
        VertexAndConstantBuffer,
        IndexBuffer,
        RenderTarget,
        UnorderedAccess,
        DepthWrite,
        DepthRead,
        NonPixelShaderResource,
        PixelShaderResource,
        StreamOut,
        IndirectArgument,
        CopyDest,
        CopySource,
        ResolveDest,
        ResolveSource,
        GenericRead,
        Present,
        Predication,
    };

    struct Descriptor
    {
        DescriptorHeap& heap;
        Data::Index     index;

        Descriptor(DescriptorHeap& in_heap, Data::Index in_index);
    };

    using DescriptorByUsage = std::map<Usage, Descriptor>;
    using BytesRange = Data::Range<Data::Index>;

    class SubResource : public Data::Chunk
    {
    public:
        class Index;

        class Count
        {
        public:
            explicit Count(Data::Size array_size  = 1U, Data::Size depth  = 1U, Data::Size mip_levels_count = 1U);

            [[nodiscard]] Data::Size GetDepth() const noexcept          { return m_depth; }
            [[nodiscard]] Data::Size GetArraySize() const noexcept      { return m_array_size; }
            [[nodiscard]] Data::Size GetMipLevelsCount() const noexcept { return m_mip_levels_count; }
            [[nodiscard]] Data::Size GetRawCount() const noexcept       { return m_depth * m_array_size * m_mip_levels_count; }

            void operator+=(const Index& other) noexcept;
            [[nodiscard]] bool operator==(const Count& other) const noexcept;
            [[nodiscard]] bool operator!=(const Count& other) const noexcept { return !operator==(other); }
            [[nodiscard]] bool operator<(const Count& other) const noexcept;
            [[nodiscard]] bool operator>=(const Count& other) const noexcept;
            [[nodiscard]] explicit operator Index() const noexcept;
            [[nodiscard]] explicit operator std::string() const noexcept;

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
            Data::Index m_depth_slice;
            Data::Index m_array_index;
            Data::Index m_mip_level;
        };

        using BytesRangeOpt = std::optional<BytesRange>;

        explicit SubResource(Data::Bytes&& data, const Index& index = Index(), BytesRangeOpt data_range = {}) noexcept;
        SubResource(Data::ConstRawPtr p_data, Data::Size size, const Index& index = Index(), BytesRangeOpt data_range = {}) noexcept;
        ~SubResource() = default;

        [[nodiscard]] const Index&         GetIndex() const noexcept             { return m_index; }
        [[nodiscard]] bool                 HasDataRange() const noexcept         { return m_data_range.has_value(); }
        [[nodiscard]] const BytesRange&    GetDataRange() const                  { return m_data_range.value(); }
        [[nodiscard]] const BytesRangeOpt& GetDataRangeOptional() const noexcept { return m_data_range; }

    private:
        Index         m_index;
        BytesRangeOpt m_data_range;
    };

    using SubResources = std::vector<SubResource>;

    class Location
    {
    public:
        Location() = default;
        Location(const Ptr<Resource>& resource_ptr, Data::Size offset = 0U) : Location(resource_ptr, SubResource::Index(), offset) { }
        Location(const Ptr<Resource>& resource_ptr, const SubResource::Index& subresource_index, Data::Size offset = 0U);

        [[nodiscard]] bool operator==(const Location& other) const noexcept;
        [[nodiscard]] bool operator!=(const Location& other) const noexcept;
        [[nodiscard]] explicit operator std::string() const;

        [[nodiscard]] bool                      IsInitialized() const noexcept       { return !!m_resource_ptr; }
        [[nodiscard]] const Ptr<Resource>&      GetResourcePtr() const noexcept      { return m_resource_ptr; }
        [[nodiscard]] Resource&                 GetResource() const;
        [[nodiscard]] const SubResource::Index& GetSubresourceIndex() const noexcept { return m_subresource_index; }
        [[nodiscard]] Data::Size                GetOffset() const noexcept           { return m_offset; }

    private:
        Ptr<Resource>      m_resource_ptr;
        SubResource::Index m_subresource_index;
        Data::Size         m_offset = 0U;
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

    class Barrier
    {
    public:
        enum class Type
        {
            Transition,
        };

        class Id
        {
        public:
            Id(Type type, const Resource& resource) noexcept;
            Id(const Id& id) noexcept = default;

            Id& operator=(const Id&) noexcept = default;

            [[nodiscard]] bool operator<(const Id& other) const noexcept;
            [[nodiscard]] bool operator==(const Id& other) const noexcept;
            [[nodiscard]] bool operator!=(const Id& other) const noexcept;

            [[nodiscard]] Type            GetType() const noexcept     { return m_type; }
            [[nodiscard]] const Resource& GetResource() const noexcept { return m_resource_ref.get(); }

        private:
            Type                m_type;
            Ref<const Resource> m_resource_ref;
        };

        class StateChange
        {
        public:
            StateChange(State before, State after) noexcept;
            StateChange(const StateChange& id) noexcept = default;

            StateChange& operator=(const StateChange&) noexcept = default;

            [[nodiscard]] bool operator<(const StateChange& other) const noexcept;
            [[nodiscard]] bool operator==(const StateChange& other) const noexcept;
            [[nodiscard]] bool operator!=(const StateChange& other) const noexcept;

            [[nodiscard]] State GetStateBefore() const noexcept { return m_before; }
            [[nodiscard]] State GetStateAfter() const noexcept  { return m_after; }

        private:
            State m_before;
            State m_after;
        };

        Barrier(const Id& id, const StateChange& state_change);
        Barrier(Type type, const Resource& resource, State state_before, State state_after);
        Barrier(const Barrier&) = default;

        Barrier& operator=(const Barrier& barrier) noexcept = default;
        [[nodiscard]] bool operator<(const Barrier& other) const noexcept;
        [[nodiscard]] bool operator==(const Barrier& other) const noexcept;
        [[nodiscard]] bool operator!=(const Barrier& other) const noexcept;
        [[nodiscard]] explicit operator std::string() const noexcept;

        [[nodiscard]] const Id&          GetId() const noexcept          { return m_id; }
        [[nodiscard]] const StateChange& GetStateChange() const noexcept { return m_state_change; }

    private:
        Id          m_id;
        StateChange m_state_change;
    };

    class Barriers
    {
    public:
        using Set = std::set<Barrier>;
        using Map = std::map<Barrier::Id, Barrier::StateChange>;

        enum class AddResult
        {
            Existing,
            Added,
            Updated
        };

        [[nodiscard]] static Ptr<Barriers> Create(const Set& barriers = {});
        [[nodiscard]] static Ptr<Barriers> CreateTransition(const Refs<const Resource>& resources, State state_before, State state_after);

        [[nodiscard]] bool       IsEmpty() const noexcept { return m_barriers_map.empty(); }
        [[nodiscard]] const Map& GetMap() const noexcept  { return m_barriers_map; }
        [[nodiscard]] Set        GetSet() const noexcept;

        [[nodiscard]] bool Has(Barrier::Type type, const Resource& resource, State before, State after);
        [[nodiscard]] bool HasTransition(const Resource& resource, State before, State after);

        AddResult Add(Barrier::Type type, const Resource& resource, State before, State after) { return AddStateChange(Barrier::Id(type, resource), Barrier::StateChange(before, after)); }
        AddResult AddTransition(const Resource& resource, State before, State after)           { return AddStateChange(Barrier::Id(Barrier::Type::Transition, resource), Barrier::StateChange(before, after));}
        bool      Remove(Barrier::Type type, const Resource& resource)                         { return Remove(Barrier::Id(type, resource)); }
        bool      RemoveTransition(const Resource& resource)                                   { return Remove(Barrier::Id(Barrier::Type::Transition, resource)); }

        virtual AddResult AddStateChange(const Barrier::Id& id, const Barrier::StateChange& state_change);
        virtual bool      Remove(const Barrier::Id& id);

        auto Lock() const { return std::scoped_lock<LockableBase(std::recursive_mutex)>(m_barriers_mutex); }

        virtual ~Barriers() = default;

        [[nodiscard]] explicit operator std::string() const noexcept;

    protected:
        explicit Barriers(const Set& barriers);

    private:
        Map m_barriers_map;
        mutable TracyLockable(std::recursive_mutex, m_barriers_mutex);
    };

    // Resource interface
    virtual bool SetState(State state, Ptr<Barriers>& out_barriers) = 0;
    virtual void SetData(const SubResources& sub_resources, CommandQueue* sync_cmd_queue = nullptr) = 0;
    [[nodiscard]] virtual SubResource               GetData(const SubResource::Index& sub_resource_index = SubResource::Index(), const std::optional<BytesRange>& data_range = {}) = 0;
    [[nodiscard]] virtual Data::Size                GetDataSize(Data::MemoryState size_type = Data::MemoryState::Reserved) const noexcept = 0;
    [[nodiscard]] virtual Data::Size                GetSubResourceDataSize(const SubResource::Index& sub_resource_index = SubResource::Index()) const = 0;
    [[nodiscard]] virtual const SubResource::Count& GetSubresourceCount() const noexcept = 0;
    [[nodiscard]] virtual Type                      GetResourceType() const noexcept = 0;
    [[nodiscard]] virtual State                     GetState() const noexcept = 0;
    [[nodiscard]] virtual Usage                     GetUsage() const noexcept = 0;
    [[nodiscard]] virtual const DescriptorByUsage&  GetDescriptorByUsage() const noexcept = 0;
    [[nodiscard]] virtual const Descriptor&         GetDescriptor(Usage usage) const = 0;
    [[nodiscard]] virtual Context&                  GetContext() noexcept = 0;
};

} // namespace Methane::Graphics

