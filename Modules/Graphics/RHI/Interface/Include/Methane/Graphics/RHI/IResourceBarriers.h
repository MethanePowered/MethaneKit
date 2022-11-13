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

FILE: Methane/Graphics/RHI/IResourceBarriers.h
Methane resource barriers for manual or automatic resource state synchronization on GPU.

******************************************************************************/

#pragma once

#include <Methane/Memory.hpp>

#include <string>
#include <map>
#include <set>

namespace Methane::Graphics::Rhi
{

struct IResource;
struct ICommandQueue;

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

struct IResourceBarriers
{
    using State = ResourceState;
    using Barrier = ResourceBarrier;
    using Set = std::set<ResourceBarrier>;
    using Map = std::map<ResourceBarrier::Id, ResourceBarrier>;

    enum class AddResult
    {
        Existing,
        Added,
        Updated
    };

    [[nodiscard]] static Ptr<IResourceBarriers> Create(const Set& barriers = {});
    [[nodiscard]] static Ptr<IResourceBarriers> CreateTransitions(const Refs<IResource>& resources,
                                                                  const Opt<Barrier::StateChange>& state_change,
                                                                  const Opt<Barrier::OwnerChange>& owner_change);

    [[nodiscard]] virtual bool  IsEmpty() const noexcept = 0;
    [[nodiscard]] virtual Set   GetSet() const noexcept = 0;
    [[nodiscard]] virtual const Map& GetMap() const noexcept = 0;
    [[nodiscard]] virtual const Barrier* GetBarrier(const Barrier::Id& id) const noexcept = 0;
    [[nodiscard]] virtual bool  HasStateTransition(IResource& resource, State before, State after) = 0;
    [[nodiscard]] virtual bool  HasOwnerTransition(IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after) = 0;
    [[nodiscard]] virtual explicit operator std::string() const noexcept = 0;

    virtual bool Remove(Barrier::Type type, IResource& resource) = 0;
    virtual bool RemoveStateTransition(IResource& resource) = 0;
    virtual bool RemoveOwnerTransition(IResource& resource) = 0;

    virtual AddResult AddStateTransition(IResource& resource, State before, State after) = 0;
    virtual AddResult AddOwnerTransition(IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after) = 0;

    virtual AddResult Add(const Barrier::Id& id, const Barrier& barrier) = 0;
    virtual bool      Remove(const Barrier::Id& id) = 0;

    virtual void ApplyTransitions() const = 0;

    virtual ~IResourceBarriers() = default;
};

} // namespace Methane::Graphics::Rhi
