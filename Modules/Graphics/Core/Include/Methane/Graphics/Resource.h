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
#include "SubResource.h"

#include <Methane/Memory.hpp>
#include <Methane/Data/IEmitter.h>
#include <Methane/Graphics/Types.h>

#include <string>
#include <vector>
#include <map>
#include <set>
#include <mutex>

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

    static constexpr Usage s_secondary_usage_mask = static_cast<Usage>(
        static_cast<uint32_t>(Usage::Addressable) |
        static_cast<uint32_t>(Usage::ReadBack)
    );

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
    using BytesRange    = Methane::Graphics::BytesRange;
    using BytesRangeOpt = Methane::Graphics::BytesRangeOpt;
    using SubResource   = Methane::Graphics::SubResource;
    using SubResources  = Methane::Graphics::SubResources;
    using Location      = Methane::Graphics::ResourceLocation;
    using Locations     = Methane::Graphics::ResourceLocations;

    template<typename TResource>
    static Locations CreateLocations(const Ptrs<TResource>& resources) { return CreateResourceLocations(resources); }

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

        void UpdateResourceStates();
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
    virtual bool SetState(State state) = 0;
    virtual bool SetState(State state, Ptr<Barriers>& out_barriers) = 0;
    virtual void SetData(const SubResources& sub_resources, CommandQueue* sync_cmd_queue = nullptr) = 0;
    [[nodiscard]] virtual SubResource               GetData(const SubResource::Index& sub_resource_index = SubResource::Index(), const BytesRangeOpt& data_range = {}) = 0;
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

