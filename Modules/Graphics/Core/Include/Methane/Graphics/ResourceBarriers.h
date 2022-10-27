/******************************************************************************

Copyright 2020-2022 Evgeny Gorodetskiy

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

struct IResource;
struct CommandQueue;

enum class ResourceState
{
    Undefined,
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

enum class ResourceBarrierType
{
    StateTransition,
    OwnerTransition,
};

class ResourceBarrierId
{
public:
    using Type = ResourceBarrierType;

    ResourceBarrierId(Type type, IResource& resource) noexcept;

    [[nodiscard]] bool operator<(const ResourceBarrierId& other) const noexcept;
    [[nodiscard]] bool operator==(const ResourceBarrierId& other) const noexcept;
    [[nodiscard]] bool operator!=(const ResourceBarrierId& other) const noexcept;

    [[nodiscard]] Type      GetType() const noexcept     { return m_type; }
    [[nodiscard]] IResource& GetResource() const noexcept { return m_resource_ref.get(); }

private:
    Type           m_type;
    Ref<IResource> m_resource_ref;
};

class ResourceStateChange
{
public:
    ResourceStateChange(ResourceState before, ResourceState after) noexcept;

    [[nodiscard]] bool operator<(const ResourceStateChange& other) const noexcept;
    [[nodiscard]] bool operator==(const ResourceStateChange& other) const noexcept;
    [[nodiscard]] bool operator!=(const ResourceStateChange& other) const noexcept;

    [[nodiscard]] ResourceState GetStateBefore() const noexcept { return m_before; }
    [[nodiscard]] ResourceState GetStateAfter() const noexcept  { return m_after; }

private:
    ResourceState m_before;
    ResourceState m_after;
};

class ResourceOwnerChange
{
public:
    using QueueFamily = uint32_t;

    ResourceOwnerChange(QueueFamily queue_family_before, QueueFamily queue_family_after) noexcept;

    [[nodiscard]] bool operator<(const ResourceOwnerChange& other) const noexcept;
    [[nodiscard]] bool operator==(const ResourceOwnerChange& other) const noexcept;
    [[nodiscard]] bool operator!=(const ResourceOwnerChange& other) const noexcept;

    [[nodiscard]] QueueFamily GetQueueFamilyBefore() const noexcept { return m_queue_family_before; }
    [[nodiscard]] QueueFamily GetQueueFamilyAfter() const noexcept  { return m_queue_family_after; }

private:
    QueueFamily m_queue_family_before;
    QueueFamily m_queue_family_after;
};

class ResourceBarrier
{
public:
    using Type        = ResourceBarrierType;
    using Id          = ResourceBarrierId;
    using StateChange = ResourceStateChange;
    using OwnerChange = ResourceOwnerChange;

    ResourceBarrier(IResource& resource, const StateChange& state_change);
    ResourceBarrier(IResource& resource, const OwnerChange& owner_change);
    ResourceBarrier(IResource& resource, ResourceState state_before, ResourceState state_after);
    ResourceBarrier(IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after);
    ResourceBarrier(const ResourceBarrier&) = default;

    ResourceBarrier& operator=(const ResourceBarrier& barrier) noexcept = default;
    [[nodiscard]] bool operator<(const ResourceBarrier& other) const noexcept;
    [[nodiscard]] bool operator==(const ResourceBarrier& other) const noexcept;
    [[nodiscard]] bool operator!=(const ResourceBarrier& other) const noexcept;
    [[nodiscard]] bool operator==(const StateChange& other) const;
    [[nodiscard]] bool operator!=(const StateChange& other) const;
    [[nodiscard]] bool operator==(const OwnerChange& other) const;
    [[nodiscard]] bool operator!=(const OwnerChange& other) const;
    [[nodiscard]] explicit operator std::string() const noexcept;

    [[nodiscard]] const Id&          GetId() const noexcept { return m_id; }
    [[nodiscard]] const StateChange& GetStateChange() const;
    [[nodiscard]] const OwnerChange& GetOwnerChange() const;

    void ApplyTransition() const;

private:
    union Change
    {
        explicit Change(const StateChange& state) : state(state) {}
        explicit Change(const OwnerChange& owner) : owner(owner) {}

        StateChange state;
        OwnerChange owner;
    };

    Id     m_id;
    Change m_change;
};

class ResourceBarriers
{
public:
    using Set = std::set<ResourceBarrier>;
    using Map = std::map<ResourceBarrier::Id, ResourceBarrier>;

    enum class AddResult
    {
        Existing,
        Added,
        Updated
    };

    [[nodiscard]] static Ptr<ResourceBarriers> Create(const Set& barriers = {});
    [[nodiscard]] static Ptr<ResourceBarriers> CreateTransitions(const Refs<IResource>& resources,
                                                                 const Opt<ResourceBarrier::StateChange>& state_change,
                                                                 const Opt<ResourceBarrier::OwnerChange>& owner_change);

    [[nodiscard]] bool  IsEmpty() const noexcept { return m_barriers_map.empty(); }
    [[nodiscard]] Set   GetSet() const noexcept;
    [[nodiscard]] const Map& GetMap() const noexcept  { return m_barriers_map; }
    [[nodiscard]] const ResourceBarrier* GetBarrier(const ResourceBarrier::Id& id) const noexcept;
    [[nodiscard]] bool  HasStateTransition(IResource& resource, ResourceState before, ResourceState after);
    [[nodiscard]] bool  HasOwnerTransition(IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after);

    bool Remove(ResourceBarrier::Type type, IResource& resource);
    bool RemoveStateTransition(IResource& resource);
    bool RemoveOwnerTransition(IResource& resource);

    AddResult AddStateTransition(IResource& resource, ResourceState before, ResourceState after);
    AddResult AddOwnerTransition(IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after);

    virtual AddResult Add(const ResourceBarrier::Id& id, const ResourceBarrier& barrier);
    virtual bool      Remove(const ResourceBarrier::Id& id);
    virtual ~ResourceBarriers() = default;

    void ApplyTransitions() const;
    auto Lock() const { return std::scoped_lock<LockableBase(std::recursive_mutex)>(m_barriers_mutex); }

    [[nodiscard]] explicit operator std::string() const noexcept;

protected:
    explicit ResourceBarriers(const Set& barriers);

private:
    Map m_barriers_map;
    mutable TracyLockable(std::recursive_mutex, m_barriers_mutex);
};

} // namespace Methane::Graphics
