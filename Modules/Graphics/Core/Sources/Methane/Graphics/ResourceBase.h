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

FILE: Methane/Graphics/ResourceBase.h
Base implementation of the resource interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Resource.h>

#include "ObjectBase.h"
#include "DescriptorHeap.h"

#include <set>
#include <map>

namespace Methane::Graphics
{

class ContextBase;

class ResourceBase
    : public virtual Resource
    , public ObjectBase
{
public:
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

    struct Barrier
    {
        enum class Type
        {
            Transition,
        };

        struct Id
        {
            const Type      type;
            const Resource& resource;

            Id(Type type, const Resource& resource);

            bool operator<(const Id& other) const noexcept;
            bool operator==(const Id& other) const noexcept;
            bool operator!=(const Id& other) const noexcept;
        };

        struct StateChange
        {
            State before;
            State after;

            StateChange(State before, State after);

            bool operator<(const StateChange& other) const noexcept;
            bool operator==(const StateChange& other) const noexcept;
            bool operator!=(const StateChange& other) const noexcept;
        };

        Id          id;
        StateChange state_change;

        Barrier(Id id, StateChange state_change);
        Barrier(Type type, const Resource& resource, State state_before, State state_after);

        bool operator<(const Barrier& other) const noexcept;
        bool operator==(const Barrier& other) const noexcept;
        bool operator!=(const Barrier& other) const noexcept;
        explicit operator std::string() const noexcept;

        static std::string GetTypeName(Type type);
    };

    class Barriers
    {
    public:
        using Set = std::set<Barrier>;
        using Map = std::map<Barrier::Id, Barrier::StateChange>;

        static Ptr<Barriers> Create(const Set& barriers = {});
        static Ptr<Barriers> CreateTransition(const Refs<const Resource>& resources, State state_before, State state_after);

        bool       IsEmpty() const noexcept { return m_barriers_map.empty(); }
        const Map& GetMap() const noexcept  { return m_barriers_map; }
        Set        GetSet() const noexcept;

        bool Has(Barrier::Type type, const Resource& resource, State before, State after);
        bool HasTransition(const Resource& resource, State before, State after);
        bool Add(Barrier::Type type, const Resource& resource, State before, State after);
        bool AddTransition(const Resource& resource, State before, State after);
        virtual bool AddStateChange(const Barrier::Id& id, const Barrier::StateChange& state_change);

        virtual ~Barriers() = default;

        explicit operator std::string() const noexcept;

    protected:
        explicit Barriers(const Set& barriers);

    private:
        Map  m_barriers_map;
    };

    ResourceBase(Type type, Usage::Mask usage_mask, ContextBase& context, DescriptorByUsage  descriptor_by_usage);
    ResourceBase(const ResourceBase&) = delete;
    ResourceBase(ResourceBase&&) = delete;
    ~ResourceBase() override;

    // Resource interface
    Type                      GetResourceType() const noexcept override          { return m_type; }
    Usage::Mask               GetUsageMask() const noexcept override             { return m_usage_mask; }
    const DescriptorByUsage&  GetDescriptorByUsage() const noexcept override     { return m_descriptor_by_usage; }
    const Descriptor&         GetDescriptor(Usage::Value usage) const override;
    void                      SetData(const SubResources& sub_resources) override;
    SubResource               GetData(const SubResource::Index& sub_resource_index = SubResource::Index(), const std::optional<BytesRange>& data_range = {}) override;
    const SubResource::Count& GetSubresourceCount() const noexcept final         { return m_sub_resource_count; }
    Data::Size                GetSubResourceDataSize(const SubResource::Index& subresource_index = SubResource::Index()) const override;
    Context&                  GetContext() noexcept override;

    void                      InitializeDefaultDescriptors();
    std::string               GetUsageNames() const noexcept                     { return Usage::ToString(m_usage_mask); }
    std::string               GetResourceTypeName() const noexcept               { return Resource::GetTypeName(m_type); }
    DescriptorHeap::Types     GetUsedDescriptorHeapTypes() const noexcept;

    State   GetState() const noexcept                                            { return m_state;  }
    bool    SetState(State state, Ptr<Barriers>& out_barriers);

    static std::string GetStateName(State state);

protected:
    ContextBase&         GetContextBase()                                       { return m_context; }
    DescriptorHeap::Type GetDescriptorHeapTypeByUsage(Usage::Value usage) const;
    const Descriptor&    GetDescriptorByUsage(Usage::Value usage) const;
    Data::Size           GetInitializedDataSize() const noexcept                { return m_initialized_data_size; }
    void                 SetSubResourceCount(const SubResource::Count& sub_resource_count);
    void                 ValidateSubResource(const SubResource& sub_resource) const;
    void                 ValidateSubResource(const SubResource::Index& sub_resource_index, const std::optional<BytesRange>& sub_resource_data_range = {}) const;

    virtual Data::Size   CalculateSubResourceDataSize(const SubResource::Index& sub_resource_index) const;

private:
    using SubResourceSizes = std::vector<Data::Size>;
    void FillSubresourceSizes();

    const Type         m_type;
    const Usage::Mask  m_usage_mask;
    ContextBase&       m_context;
    DescriptorByUsage  m_descriptor_by_usage;
    State              m_state = State::Common;
    Data::Size         m_initialized_data_size       = 0U;
    bool               m_sub_resource_count_constant = false;
    SubResource::Count m_sub_resource_count;
    SubResourceSizes   m_sub_resource_sizes;
};

} // namespace Methane::Graphics
