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

FILE: Methane/Graphics/ResourceBarriers.h
Methane resource barriers for manual or automatic resource state synchronization on GPU.

******************************************************************************/

#pragma once

#include <Methane/Memory.hpp>

#include <Tracy.hpp>

#include <string>
#include <mutex>
#include <map>
#include <set>

namespace Methane::Graphics
{

struct Resource;

enum class ResourceState
{
    Common,
    VertexBuffer,
    ConstantBuffer,
    IndexBuffer,
    RenderTarget,
    InputAttachment,
    UnorderedAccess,
    DepthWrite,
    DepthRead,
    ShaderResource,
    StreamOut,
    IndirectArgument,
    CopyDest,
    CopySource,
    ResolveDest,
    ResolveSource,
    GenericRead,
    Present,
};

class ResourceBarrier
{
public:
    enum class Type
    {
        Transition,
    };

    class Id
    {
    public:
        Id(Type type, Resource& resource) noexcept;
        Id(const Id& id) noexcept = default;

        Id& operator=(const Id&) noexcept = default;

        [[nodiscard]] bool operator<(const Id& other) const noexcept;
        [[nodiscard]] bool operator==(const Id& other) const noexcept;
        [[nodiscard]] bool operator!=(const Id& other) const noexcept;

        [[nodiscard]] Type      GetType() const noexcept     { return m_type; }
        [[nodiscard]] Resource& GetResource() const noexcept { return m_resource_ref.get(); }

    private:
        Type                m_type;
        Ref<Resource> m_resource_ref;
    };

    class StateChange
    {
    public:
        StateChange(ResourceState before, ResourceState after) noexcept;
        StateChange(const StateChange& id) noexcept = default;

        StateChange& operator=(const StateChange&) noexcept = default;

        [[nodiscard]] bool operator<(const StateChange& other) const noexcept;
        [[nodiscard]] bool operator==(const StateChange& other) const noexcept;
        [[nodiscard]] bool operator!=(const StateChange& other) const noexcept;

        [[nodiscard]] ResourceState GetStateBefore() const noexcept { return m_before; }
        [[nodiscard]] ResourceState GetStateAfter() const noexcept  { return m_after; }

    private:
        ResourceState m_before;
        ResourceState m_after;
    };

    ResourceBarrier(const Id& id, const StateChange& state_change);
    ResourceBarrier(Type type, Resource& resource, ResourceState state_before, ResourceState state_after);
    ResourceBarrier(const ResourceBarrier&) = default;

    ResourceBarrier& operator=(const ResourceBarrier& barrier) noexcept = default;
    [[nodiscard]] bool operator<(const ResourceBarrier& other) const noexcept;
    [[nodiscard]] bool operator==(const ResourceBarrier& other) const noexcept;
    [[nodiscard]] bool operator!=(const ResourceBarrier& other) const noexcept;
    [[nodiscard]] explicit operator std::string() const noexcept;

    [[nodiscard]] const Id&          GetId() const noexcept          { return m_id; }
    [[nodiscard]] const StateChange& GetStateChange() const noexcept { return m_state_change; }

private:
    Id          m_id;
    StateChange m_state_change;
};

class ResourceBarriers
{
public:
    using Set = std::set<ResourceBarrier>;
    using Map = std::map<ResourceBarrier::Id, ResourceBarrier::StateChange>;

    enum class AddResult
    {
        Existing,
        Added,
        Updated
    };

    [[nodiscard]] static Ptr<ResourceBarriers> Create(const Set& barriers = {});
    [[nodiscard]] static Ptr<ResourceBarriers> CreateTransition(const Refs<Resource>& resources, ResourceState state_before, ResourceState state_after);

    [[nodiscard]] bool       IsEmpty() const noexcept { return m_barriers_map.empty(); }
    [[nodiscard]] const Map& GetMap() const noexcept  { return m_barriers_map; }
    [[nodiscard]] Set        GetSet() const noexcept;

    [[nodiscard]] bool Has(ResourceBarrier::Type type, Resource& resource, ResourceState before, ResourceState after);
    [[nodiscard]] bool HasTransition(Resource& resource, ResourceState before, ResourceState after);

    AddResult Add(ResourceBarrier::Type type, Resource& resource, ResourceState before, ResourceState after) { return AddStateChange(ResourceBarrier::Id(type, resource), ResourceBarrier::StateChange(before, after)); }
    AddResult AddTransition(Resource& resource, ResourceState before, ResourceState after)                   { return AddStateChange(ResourceBarrier::Id(ResourceBarrier::Type::Transition, resource), ResourceBarrier::StateChange(before, after)); }
    bool      Remove(ResourceBarrier::Type type, Resource& resource)                                         { return Remove(ResourceBarrier::Id(type, resource)); }
    bool      RemoveTransition(Resource& resource)                                                           { return Remove(ResourceBarrier::Id(ResourceBarrier::Type::Transition, resource)); }

    virtual AddResult AddStateChange(const ResourceBarrier::Id& id, const ResourceBarrier::StateChange& state_change);
    virtual bool      Remove(const ResourceBarrier::Id& id);
    virtual ~ResourceBarriers() = default;

    void UpdateResourceStates() const;
    auto Lock() const { return std::scoped_lock<LockableBase(std::recursive_mutex)>(m_barriers_mutex); }

    [[nodiscard]] explicit operator std::string() const noexcept;

protected:
    explicit ResourceBarriers(const Set& barriers);

private:
    Map m_barriers_map;
    mutable TracyLockable(std::recursive_mutex, m_barriers_mutex);
};

} // namespace Methane::Graphics
