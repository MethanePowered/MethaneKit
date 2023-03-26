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

FILE: Methane/Graphics/RHI/IResource.h
Methane resource interface: base class of all GPU resources.

******************************************************************************/

#pragma once

#include "IObject.h"
#include "ResourceView.h"
#include "IResourceBarriers.h"
#include "IDescriptorManager.h"

#include <Methane/Memory.hpp>
#include <Methane/Data/IEmitter.h>
#include <Methane/Graphics/Types.h>

#include <string_view>

namespace Methane::Graphics::Rhi
{

enum class ResourceType
{
    Buffer,
    Texture,
    Sampler,
};

struct IResource;

class ResourceAllocationError : public std::runtime_error
{
public:
    ResourceAllocationError(const IResource& resource, std::string_view error_message);

    const IResource& GetResource() const noexcept { return m_resource; }

private:
    const IResource& m_resource;
};

struct IResourceCallback
{
    virtual void OnResourceReleased(IResource& resource) = 0;

    virtual ~IResourceCallback() = default;
};

struct IContext;
struct ICommandQueue;

struct IResource
    : virtual IObject // NOSONAR
    , virtual Data::IEmitter<IResourceCallback> // NOSONAR
{
    using Usage              = ResourceUsage;
    using UsageMask          = ResourceUsageMask;
    using Type               = ResourceType;
    using Descriptor         = DirectX::ResourceDescriptor;
    using DescriptorByViewId = std::map<ResourceView::Id, Descriptor>;
    using AllocationError    = ResourceAllocationError;
    using State              = ResourceState;
    using View               = ResourceView;
    using Views              = ResourceViews;
    using Barrier            = ResourceBarrier;
    using IBarriers          = IResourceBarriers;
    using BytesRange         = Rhi::BytesRange;
    using BytesRangeOpt      = Rhi::BytesRangeOpt;
    using SubResource        = Rhi::SubResource;
    using SubResources       = Rhi::SubResources;

    template<typename TResource>
    static Views CreateViews(const Ptrs<TResource>& resources) { return CreateResourceViews(resources); }

    // IResource interface
    virtual bool SetState(State state) = 0;
    virtual bool SetState(State state, Ptr<IBarriers>& out_barriers) = 0;
    virtual bool SetOwnerQueueFamily(uint32_t family_index) = 0;
    virtual bool SetOwnerQueueFamily(uint32_t family_index, Ptr<IBarriers>& out_barriers) = 0;
    virtual void RestoreDescriptorViews(const DescriptorByViewId& descriptor_by_view_id) = 0;
    [[nodiscard]] virtual Data::Size                GetDataSize(Data::MemoryState size_type = Data::MemoryState::Reserved) const noexcept = 0;
    [[nodiscard]] virtual Data::Size                GetSubResourceDataSize(const SubResource::Index& sub_resource_index = {}) const = 0;
    [[nodiscard]] virtual SubResource::Count        GetSubresourceCount() const noexcept = 0;
    [[nodiscard]] virtual Type                      GetResourceType() const noexcept = 0;
    [[nodiscard]] virtual State                     GetState() const noexcept = 0;
    [[nodiscard]] virtual UsageMask                 GetUsage() const noexcept = 0;
    [[nodiscard]] virtual const DescriptorByViewId& GetDescriptorByViewId() const noexcept = 0;
    [[nodiscard]] virtual const IContext&           GetContext() const noexcept = 0;
    [[nodiscard]] virtual const Opt<uint32_t>&      GetOwnerQueueFamily() const noexcept = 0;
};

} // namespace Methane::Graphics::Rhi

