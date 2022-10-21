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

#include "IObject.h"
#include "ResourceView.h"
#include "ResourceBarriers.h"

#include <Methane/Memory.hpp>
#include <Methane/Data/IEmitter.h>
#include <Methane/Graphics/Types.h>

#include <string_view>

namespace Methane::Graphics
{

struct IContext;
struct CommandQueue;
struct Resource;
class DescriptorHeapDX;

struct IResourceCallback
{
    virtual void OnResourceReleased(Resource& resource) = 0;

    virtual ~IResourceCallback() = default;
};

struct Resource
    : virtual IObject // NOSONAR
    , virtual Data::IEmitter<IResourceCallback> // NOSONAR
{
    using Usage = ResourceUsage;

    enum class Type
    {
        Buffer,
        Texture,
        Sampler,
    };

    struct Descriptor
    {
        DescriptorHeapDX& heap;
        Data::Index       index;

        Descriptor(DescriptorHeapDX& in_heap, Data::Index in_index);
    };

    using DescriptorByViewId = std::map<ResourceView::Id, Descriptor>;

    class AllocationError : public std::runtime_error
    {
    public:
        AllocationError(const Resource& resource, std::string_view error_message);

        const Resource& GetResource() const noexcept { return m_resource; }

    private:
        const Resource& m_resource;
    };

    using State         = ResourceState;
    using BytesRange    = Methane::Graphics::BytesRange;
    using BytesRangeOpt = Methane::Graphics::BytesRangeOpt;
    using SubResource   = Methane::Graphics::SubResource;
    using SubResources  = Methane::Graphics::SubResources;
    using View          = Methane::Graphics::ResourceView;
    using Views         = Methane::Graphics::ResourceViews;
    using Barrier       = Methane::Graphics::ResourceBarrier;
    using Barriers      = Methane::Graphics::ResourceBarriers;

    template<typename TResource>
    static Views CreateViews(const Ptrs<TResource>& resources) { return CreateResourceViews(resources); }

    // Resource interface
    virtual bool SetState(State state) = 0;
    virtual bool SetState(State state, Ptr<Barriers>& out_barriers) = 0;
    virtual bool SetOwnerQueueFamily(uint32_t family_index) = 0;
    virtual bool SetOwnerQueueFamily(uint32_t family_index, Ptr<Barriers>& out_barriers) = 0;
    virtual void SetData(const SubResources& sub_resources, CommandQueue& target_cmd_queue) = 0;
    virtual void RestoreDescriptorViews(const DescriptorByViewId& descriptor_by_view_id) = 0;

    [[nodiscard]] virtual SubResource               GetData(const SubResource::Index& sub_resource_index = SubResource::Index(), const BytesRangeOpt& data_range = {}) = 0;
    [[nodiscard]] virtual Data::Size                GetDataSize(Data::MemoryState size_type = Data::MemoryState::Reserved) const noexcept = 0;
    [[nodiscard]] virtual Data::Size                GetSubResourceDataSize(const SubResource::Index& sub_resource_index = SubResource::Index()) const = 0;
    [[nodiscard]] virtual const SubResource::Count& GetSubresourceCount() const noexcept = 0;
    [[nodiscard]] virtual Type                      GetResourceType() const noexcept = 0;
    [[nodiscard]] virtual State                     GetState() const noexcept = 0;
    [[nodiscard]] virtual Usage                     GetUsage() const noexcept = 0;
    [[nodiscard]] virtual const DescriptorByViewId& GetDescriptorByViewId() const noexcept = 0;
    [[nodiscard]] virtual const IContext&            GetContext() const noexcept = 0;
    [[nodiscard]] virtual const Opt<uint32_t>&      GetOwnerQueueFamily() const noexcept = 0;
};

} // namespace Methane::Graphics

