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

FILE: Methane/Graphics/RHI/Buffer.h
Methane Buffer PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include "Pimpl.h"

#include <Methane/Graphics/RHI/IBuffer.h>
#include <Methane/Data/Transmitter.hpp>

#include <vector>

namespace Methane::Graphics::Rhi
{

class RenderContext;
class ResourceBarriers;
class CommandQueue;

class Buffer
    : public Data::Transmitter<Rhi::IObjectCallback>
    , public Data::Transmitter<Rhi::IResourceCallback>
{
public:
    using AllocationError = ResourceAllocationError;
    using State           = ResourceState;
    using View            = ResourceView;
    using Views           = ResourceViews;
    using Barrier         = ResourceBarrier;
    using Barriers        = ResourceBarriers;
    using Type            = BufferType;
    using StorageMode     = BufferStorageMode;
    using Settings        = BufferSettings;

    using Descriptor         = DirectX::ResourceDescriptor;
    using DescriptorByViewId = std::map<ResourceView::Id, Descriptor>;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(Buffer);
    META_PIMPL_METHODS_COMPARE_DECLARE(Buffer);

    Buffer(const Ptr<IBuffer>& interface_ptr);
    Buffer(IBuffer& interface_ref);

    void InitVertexBuffer(const IContext& context, Data::Size size, Data::Size stride, bool is_volatile = false);
    void InitIndexBuffer(const IContext& context, Data::Size size, PixelFormat format, bool is_volatile = false);
    void InitConstantBuffer(const IContext& context, Data::Size size, bool addressable = false, bool is_volatile = false);
    void InitReadBackBuffer(const IContext& context, Data::Size size);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    IBuffer& GetInterface() const META_PIMPL_NOEXCEPT;
    Ptr<IBuffer> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    bool SetName(std::string_view name) const;
    std::string_view GetName() const META_PIMPL_NOEXCEPT;

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
    [[nodiscard]] const IContext&           GetContext() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] const Opt<uint32_t>&      GetOwnerQueueFamily() const META_PIMPL_NOEXCEPT;

    // IBuffer interface methods
    [[nodiscard]] const Settings& GetSettings() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] uint32_t GetFormattedItemsCount() const META_PIMPL_NOEXCEPT;
    
private:
    class Impl;

    Buffer(ImplPtr<Impl>&& impl_ptr);

    ImplPtr<Impl> m_impl_ptr;
};

class BufferSet
    : public Data::Transmitter<Rhi::IObjectCallback>
{
public:
    using Buffers = std::vector<Buffer>;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(BufferSet);
    META_PIMPL_METHODS_COMPARE_DECLARE(BufferSet);

    BufferSet(const Ptr<IBufferSet>& interface_ptr);
    BufferSet(IBufferSet& interface_ref);
    BufferSet(BufferType buffers_type, const Refs<Buffer>& buffer_refs);

    void Init(BufferType buffers_type, const Refs<Buffer>& buffer_refs);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    IBufferSet& GetInterface() const META_PIMPL_NOEXCEPT;
    Ptr<IBufferSet> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IBufferSet interface methods
    [[nodiscard]] BufferType     GetType() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] Data::Size     GetCount() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] const Buffers& GetRefs() const noexcept;
    [[nodiscard]] std::string    GetNames() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] const Buffer&  operator[](Data::Index index) const;

private:
    class Impl;

    BufferSet(ImplPtr<Impl>&& impl_ptr);

    ImplPtr<Impl> m_impl_ptr;
    mutable Buffers m_buffers;
};

} // namespace Methane::Graphics::Rhi
