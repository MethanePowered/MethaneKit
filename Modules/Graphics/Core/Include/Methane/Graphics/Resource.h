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
#include "ResourceBarriers.h"

#include <Methane/Memory.hpp>
#include <Methane/Data/IEmitter.h>
#include <Methane/Graphics/Types.h>

namespace Methane::Graphics
{

struct Context;
struct CommandQueue;
struct Resource;
class DescriptorHeap;

struct IResourceCallback
{
    virtual void OnResourceReleased(Resource& resource) = 0;

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

    struct Descriptor
    {
        DescriptorHeap& heap;
        Data::Index     index;

        Descriptor(DescriptorHeap& in_heap, Data::Index in_index);
    };

    using State         = ResourceState;
    using DescriptorByUsage = std::map<Usage, Descriptor>;
    using BytesRange    = Methane::Graphics::BytesRange;
    using BytesRangeOpt = Methane::Graphics::BytesRangeOpt;
    using SubResource   = Methane::Graphics::SubResource;
    using SubResources  = Methane::Graphics::SubResources;
    using Location      = Methane::Graphics::ResourceLocation;
    using Locations     = Methane::Graphics::ResourceLocations;
    using Barrier       = Methane::Graphics::ResourceBarrier;
    using Barriers      = Methane::Graphics::ResourceBarriers;

    template<typename TResource>
    static Locations CreateLocations(const Ptrs<TResource>& resources) { return CreateResourceLocations(resources); }

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
    [[nodiscard]] virtual const Context&            GetContext() const noexcept = 0;
};

} // namespace Methane::Graphics

