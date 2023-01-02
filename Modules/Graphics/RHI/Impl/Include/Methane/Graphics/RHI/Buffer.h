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

#include <vector>

namespace Methane::Graphics::META_GFX_NAME
{
class Buffer;
class BufferSet;
}

namespace Methane::Graphics::Rhi
{

class RenderContext;
class ResourceBarriers;
class CommandQueue;

class Buffer
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

    META_RHI_API explicit Buffer(const Ptr<IBuffer>& interface_ptr);
    META_RHI_API explicit Buffer(IBuffer& interface_ref);

    META_RHI_API void InitVertexBuffer(const IContext& context, Data::Size size, Data::Size stride, bool is_volatile = false);
    META_RHI_API void InitIndexBuffer(const IContext& context, Data::Size size, PixelFormat format, bool is_volatile = false);
    META_RHI_API void InitConstantBuffer(const IContext& context, Data::Size size, bool addressable = false, bool is_volatile = false);
    META_RHI_API void InitReadBackBuffer(const IContext& context, Data::Size size);
    META_RHI_API void Release();

    META_RHI_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_RHI_API IBuffer& GetInterface() const META_PIMPL_NOEXCEPT;
    META_RHI_API Ptr<IBuffer> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_RHI_API bool SetName(std::string_view name) const;
    META_RHI_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_RHI_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_RHI_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // IResource interface methods
    META_RHI_API bool SetState(State state) const;
    META_RHI_API bool SetState(State state, Barriers& out_barriers) const;
    META_RHI_API bool SetOwnerQueueFamily(uint32_t family_index) const;
    META_RHI_API bool SetOwnerQueueFamily(uint32_t family_index, Barriers& out_barriers) const;
    META_RHI_API void SetData(const SubResources& sub_resources, const CommandQueue& target_cmd_queue) const;
    META_RHI_API void RestoreDescriptorViews(const DescriptorByViewId& descriptor_by_view_id) const;

    [[nodiscard]] META_RHI_API SubResource               GetData(const SubResource::Index& sub_resource_index = SubResource::Index(), const BytesRangeOpt& data_range = {}) const;
    [[nodiscard]] META_RHI_API Data::Size                GetDataSize(Data::MemoryState size_type = Data::MemoryState::Reserved) const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API Data::Size                GetSubResourceDataSize(const SubResource::Index& sub_resource_index = SubResource::Index()) const;
    [[nodiscard]] META_RHI_API const SubResource::Count& GetSubresourceCount() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API ResourceType              GetResourceType() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API State                     GetState() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API ResourceUsageMask         GetUsage() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API const DescriptorByViewId& GetDescriptorByViewId() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API const IContext&           GetContext() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API const Opt<uint32_t>&      GetOwnerQueueFamily() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IResourceCallback> interface methods
    META_RHI_API void Connect(Data::Receiver<IResourceCallback>& receiver) const;
    META_RHI_API void Disconnect(Data::Receiver<IResourceCallback>& receiver) const;

    // IBuffer interface methods
    [[nodiscard]] META_RHI_API const Settings& GetSettings() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API uint32_t GetFormattedItemsCount() const META_PIMPL_NOEXCEPT;
    
private:
    using Impl = Methane::Graphics::META_GFX_NAME::Buffer;

    META_RHI_API Buffer(Ptr<Impl>&& impl_ptr);

    Ptr<Impl> m_impl_ptr;
};

class BufferSet
{
public:
    using Buffers = std::vector<Buffer>;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(BufferSet);
    META_PIMPL_METHODS_COMPARE_DECLARE(BufferSet);

    META_RHI_API explicit BufferSet(const Ptr<IBufferSet>& interface_ptr);
    META_RHI_API explicit BufferSet(IBufferSet& interface_ref);
    META_RHI_API BufferSet(BufferType buffers_type, const Refs<Buffer>& buffer_refs);

    META_RHI_API void Init(BufferType buffers_type, const Refs<Buffer>& buffer_refs);
    META_RHI_API void Release();

    META_RHI_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_RHI_API IBufferSet& GetInterface() const META_PIMPL_NOEXCEPT;
    META_RHI_API Ptr<IBufferSet> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_RHI_API bool SetName(std::string_view name) const;
    META_RHI_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_RHI_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_RHI_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // IBufferSet interface methods
    [[nodiscard]] META_RHI_API BufferType     GetType() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API Data::Size     GetCount() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API const Buffers& GetRefs() const noexcept;
    [[nodiscard]] META_RHI_API std::string    GetNames() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API const Buffer&  operator[](Data::Index index) const;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::BufferSet;

    META_RHI_API BufferSet(Ptr<Impl>&& impl_ptr);

    Ptr<Impl> m_impl_ptr;
    mutable Buffers m_buffers;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_RHI_PIMPL_INLINE

#include <Methane/Graphics/RHI/Buffer.cpp>

#endif // META_RHI_PIMPL_INLINE
