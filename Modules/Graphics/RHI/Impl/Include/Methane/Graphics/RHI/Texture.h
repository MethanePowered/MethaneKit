/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/Texture.h
Methane Texture PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include "Pimpl.h"

#include <Methane/Graphics/RHI/ITexture.h>

#include <vector>

namespace Methane::Graphics::Rhi
{

class RenderContext;
class ResourceBarriers;
class CommandQueue;

class ImageTexture
{
public:
    using AllocationError  = ResourceAllocationError;
    using State            = ResourceState;
    using Barrier          = ResourceBarrier;
    using Barriers         = ResourceBarriers;
    using Type             = TextureType;
    using DimensionType    = TextureDimensionType;
    using View             = TextureView;
    using Views            = TextureViews;
    using Settings         = TextureSettings;

    using Descriptor         = DirectX::ResourceDescriptor;
    using DescriptorByViewId = std::map<ResourceView::Id, Descriptor>;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(ImageTexture);

    ImageTexture(const Ptr<ITexture>& interface_ptr);
    ImageTexture(ITexture& interface_ref);
    ImageTexture(const RenderContext& context, const Dimensions& dimensions, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped);

    void Init(const RenderContext& context, const Dimensions& dimensions, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    ITexture& GetInterface() const META_PIMPL_NOEXCEPT;

    // IResource interface methods
    bool SetState(State state) const;
    bool SetState(State state, Barriers& out_barriers) const;
    bool SetOwnerQueueFamily(uint32_t family_index) const;
    bool SetOwnerQueueFamily(uint32_t family_index, Barriers& out_barriers) const;
    void SetData(const SubResources& sub_resources, const CommandQueue& target_cmd_queue) const;
    void RestoreDescriptorViews(const DescriptorByViewId& descriptor_by_view_id) const;

    [[nodiscard]] SubResource               GetData(const SubResource::Index& sub_resource_index = SubResource::Index(), const BytesRangeOpt& data_range = {}) const;
    [[nodiscard]] Data::Size                GetDataSize(Data::MemoryState size_type = Data::MemoryState::Reserved) const META_PIMPL_NOEXCEPT;
    [[nodiscard]] Data::Size                GetSubResourceDataSize(const SubResource::Index& sub_resource_index = SubResource::Index()) const;
    [[nodiscard]] const SubResource::Count& GetSubresourceCount() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] ResourceType              GetResourceType() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] State                     GetState() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] ResourceUsageMask         GetUsage() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] const DescriptorByViewId& GetDescriptorByViewId() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] RenderContext             GetRenderContext() const;
    [[nodiscard]] const Opt<uint32_t>&      GetOwnerQueueFamily() const META_PIMPL_NOEXCEPT;

    // ITexture interface methods
    [[nodiscard]] const Settings& GetSettings() const META_PIMPL_NOEXCEPT;
    
private:
    class Impl;

    UniquePtr<Impl> m_impl_ptr;
};

class FrameBufferTexture
{
public:
    using AllocationError  = ResourceAllocationError;
    using State            = ResourceState;
    using Barrier          = ResourceBarrier;
    using Barriers         = ResourceBarriers;
    using Type             = TextureType;
    using DimensionType    = TextureDimensionType;
    using View             = TextureView;
    using Views            = TextureViews;
    using Settings         = TextureSettings;
    using FrameBufferIndex = uint32_t;

    using Descriptor         = DirectX::ResourceDescriptor;
    using DescriptorByViewId = std::map<ResourceView::Id, Descriptor>;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(FrameBufferTexture);

    FrameBufferTexture(const Ptr<ITexture>& interface_ptr);
    FrameBufferTexture(ITexture& interface_ref);
    FrameBufferTexture(const RenderContext& context, FrameBufferIndex frame_buffer_index);

    void Init(const RenderContext& context, FrameBufferIndex frame_buffer_index);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    ITexture& GetInterface() const META_PIMPL_NOEXCEPT;

    // IResource interface methods
    bool SetState(State state) const;
    bool SetState(State state, Barriers& out_barriers) const;
    bool SetOwnerQueueFamily(uint32_t family_index) const;
    bool SetOwnerQueueFamily(uint32_t family_index, Barriers& out_barriers) const;
    void RestoreDescriptorViews(const DescriptorByViewId& descriptor_by_view_id) const;

    [[nodiscard]] SubResource               GetData(const SubResource::Index& sub_resource_index = SubResource::Index(), const BytesRangeOpt& data_range = {}) const;
    [[nodiscard]] Data::Size                GetDataSize(Data::MemoryState size_type = Data::MemoryState::Reserved) const META_PIMPL_NOEXCEPT;
    [[nodiscard]] Data::Size                GetSubResourceDataSize(const SubResource::Index& sub_resource_index = SubResource::Index()) const;
    [[nodiscard]] const SubResource::Count& GetSubresourceCount() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] ResourceType              GetResourceType() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] State                     GetState() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] ResourceUsageMask         GetUsage() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] const DescriptorByViewId& GetDescriptorByViewId() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] RenderContext             GetRenderContext() const;
    [[nodiscard]] const Opt<uint32_t>&      GetOwnerQueueFamily() const META_PIMPL_NOEXCEPT;

    // ITexture interface methods
    [[nodiscard]] const Settings& GetSettings() const META_PIMPL_NOEXCEPT;

private:
    class Impl;

    UniquePtr<Impl> m_impl_ptr;
};

class DepthStencilTexture
{
public:
    using AllocationError  = ResourceAllocationError;
    using State            = ResourceState;
    using Barrier          = ResourceBarrier;
    using Barriers         = ResourceBarriers;
    using Type             = TextureType;
    using DimensionType    = TextureDimensionType;
    using View             = TextureView;
    using Views            = TextureViews;
    using Settings         = TextureSettings;
    using FrameBufferIndex = uint32_t;

    using Descriptor         = DirectX::ResourceDescriptor;
    using DescriptorByViewId = std::map<ResourceView::Id, Descriptor>;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(DepthStencilTexture);

    DepthStencilTexture(const Ptr<ITexture>& interface_ptr);
    DepthStencilTexture(ITexture& interface_ref);
    DepthStencilTexture(const RenderContext& context);

    void Init(const RenderContext& context);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    ITexture& GetInterface() const META_PIMPL_NOEXCEPT;

    // IResource interface methods
    bool SetState(State state) const;
    bool SetState(State state, Barriers& out_barriers) const;
    bool SetOwnerQueueFamily(uint32_t family_index) const;
    bool SetOwnerQueueFamily(uint32_t family_index, Barriers& out_barriers) const;
    void RestoreDescriptorViews(const DescriptorByViewId& descriptor_by_view_id) const;

    [[nodiscard]] SubResource               GetData(const SubResource::Index& sub_resource_index = SubResource::Index(), const BytesRangeOpt& data_range = {}) const;
    [[nodiscard]] Data::Size                GetDataSize(Data::MemoryState size_type = Data::MemoryState::Reserved) const META_PIMPL_NOEXCEPT;
    [[nodiscard]] Data::Size                GetSubResourceDataSize(const SubResource::Index& sub_resource_index = SubResource::Index()) const;
    [[nodiscard]] const SubResource::Count& GetSubresourceCount() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] ResourceType              GetResourceType() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] State                     GetState() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] ResourceUsageMask         GetUsage() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] const DescriptorByViewId& GetDescriptorByViewId() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] RenderContext             GetRenderContext() const;
    [[nodiscard]] const Opt<uint32_t>&      GetOwnerQueueFamily() const META_PIMPL_NOEXCEPT;

    // ITexture interface methods
    [[nodiscard]] const Settings& GetSettings() const META_PIMPL_NOEXCEPT;

private:
    class Impl;

    UniquePtr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi
