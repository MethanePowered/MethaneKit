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

FILE: Methane/Graphics/ResourceBase.h
Base implementation of the resource interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Resource.h>

#include "ObjectBase.h"
#include "DescriptorHeap.h"

namespace Methane::Graphics
{

class ContextBase;

class ResourceBase
    : public virtual Resource
    , public ObjectBase
    , public std::enable_shared_from_this<ResourceBase>
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

        Type      type;
        Resource& resource;
        State     state_before;
        State     state_after;

        Barrier(Type type, Resource& resource, State state_before, State state_after);
    };

    class Barriers
    {
    public:
        static Ptr<Barriers> Create(std::vector<Barrier> barriers = {});
        static Ptr<Barriers> CreateTransition(const Refs<Resource>& resources, State state_before, State state_after);

        bool IsEmpty() const noexcept { return m_barriers.empty(); }
        const std::vector<Barrier>& Get() const noexcept { return m_barriers; }

        virtual const Barrier& Add(Barrier::Type type, Resource& resource, State state_before, State state_after);
        virtual ~Barriers() = default;

    protected:
        Barriers(std::vector<Barrier> barriers);

    private:
        std::vector<Barrier> m_barriers;
    };

    struct ReleasePool
    {
        static Ptr<ReleasePool> Create();

        virtual void AddResource(ResourceBase& resource) = 0;
        virtual void ReleaseResources() = 0;

        virtual ~ReleasePool() = default;
    };

    ResourceBase(Type type, Usage::Mask usage_mask, ContextBase& context, DescriptorByUsage  descriptor_by_usage);
    ~ResourceBase() override;

    // Resource interface
    Type                      GetResourceType() const noexcept override          { return m_type; }
    Usage::Mask               GetUsageMask() const noexcept override             { return m_usage_mask; }
    const DescriptorByUsage&  GetDescriptorByUsage() const noexcept override     { return m_descriptor_by_usage; }
    const Descriptor&         GetDescriptor(Usage::Value usage) const override;
    void                      SetData(const SubResources& sub_resources) override;
    SubResource               GetData(const BytesRange& data_range = BytesRange()) override;
    const SubResource::Count& GetSubresourceCount() const noexcept override      { return m_subresource_count; }
    const BytesRange&         GetSubresourceDataRange(const SubResource::Index& subresource_index = SubResource::Index()) const override;

    void                      InitializeDefaultDescriptors();
    std::string               GetUsageNames() const noexcept                     { return Usage::ToString(m_usage_mask); }
    std::string               GetResourceTypeName() const noexcept               { return Resource::GetTypeName(m_type); }
    DescriptorHeap::Types     GetUsedDescriptorHeapTypes() const noexcept;

    State   GetState() const noexcept                                            { return m_state;  }
    void    SetState(State state, Ptr<Barriers>& out_barriers);

protected:
    DescriptorHeap::Type GetDescriptorHeapTypeByUsage(Usage::Value usage) const;
    const Descriptor&    GetDescriptorByUsage(Usage::Value usage) const;
    ContextBase&         GetContext() { return m_context; }
    Data::Size           GetInitializedDataSize() const noexcept { return m_initialized_data_size; }
    void                 SetSubresourceCount(const SubResource::Count& sub_resource_count);

    virtual Data::Size   GetSubresourceDataSize(const SubResource::Index& subresource_index) const;

private:
    void FillSubresourceRanges();

    using SubResourceRanges = std::vector<BytesRange>;

    const Type          m_type;
    const Usage::Mask   m_usage_mask;
    ContextBase&        m_context;
    DescriptorByUsage   m_descriptor_by_usage;
    State               m_state = State::Common;
    Data::Size          m_initialized_data_size = 0u;
    bool                m_subresource_count_constant = false;
    SubResource::Count  m_subresource_count;
    SubResourceRanges   m_subresource_ranges;
};

} // namespace Methane::Graphics
