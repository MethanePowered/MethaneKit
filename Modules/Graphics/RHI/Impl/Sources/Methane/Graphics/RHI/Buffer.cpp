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

FILE: Methane/Graphics/RHI/Buffer.cpp
Methane Buffer PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/Buffer.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/ResourceBarriers.h>

#if defined METHANE_GFX_DIRECTX

#include <Methane/Graphics/DirectX/Buffer.h>
using BufferImpl = Methane::Graphics::DirectX::Buffer;
using BufferSetImpl = Methane::Graphics::DirectX::BufferSet;

#elif defined METHANE_GFX_VULKAN

#include <Methane/Graphics/Vulkan/Buffer.h>
using BufferImpl = Methane::Graphics::Vulkan::Buffer;
using BufferSetImpl = Methane::Graphics::Vulkan::BufferSet;

#elif defined METHANE_GFX_METAL

#include <Methane/Graphics/Metal/Buffer.hh>
using BufferImpl = Methane::Graphics::Metal::Buffer;
using BufferSetImpl = Methane::Graphics::Metal::BufferSet;

#else // METHAN_GFX_[API] is undefined

static_assert(false, "Static graphics API macro-definition is missing.");

#endif

#include "ImplWrapper.hpp"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

class Buffer::Impl
    : public ImplWrapper<IBuffer, BufferImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(Buffer);

Buffer::Buffer(UniquePtr<Impl>&& impl_ptr)
    : Transmitter<IObjectCallback>(impl_ptr->GetInterface())
    , Transmitter<IResourceCallback>(impl_ptr->GetInterface())
    , m_impl_ptr(std::move(impl_ptr))
{
}

Buffer::Buffer(const Ptr<IBuffer>& interface_ptr)
    : Buffer(std::make_unique<Impl>(interface_ptr))
{
}

Buffer::Buffer(IBuffer& interface_ref)
    : Buffer(interface_ref.GetDerivedPtr<IBuffer>())
{
}

void Buffer::InitVertexBuffer(const RenderContext& context, Data::Size size, Data::Size stride, bool is_volatile)
{
    m_impl_ptr = std::make_unique<Impl>(IBuffer::CreateVertexBuffer(context.GetInterface(), size, stride, is_volatile));
    Transmitter<IObjectCallback>::Reset(&m_impl_ptr->GetInterface());
    Transmitter<IResourceCallback>::Reset(&m_impl_ptr->GetInterface());
}

void Buffer::InitIndexBuffer(const RenderContext& context, Data::Size size, PixelFormat format, bool is_volatile)
{
    m_impl_ptr = std::make_unique<Impl>(IBuffer::CreateIndexBuffer(context.GetInterface(), size, format, is_volatile));
    Transmitter<IObjectCallback>::Reset(&m_impl_ptr->GetInterface());
    Transmitter<IResourceCallback>::Reset(&m_impl_ptr->GetInterface());
}

void Buffer::InitConstantBuffer(const RenderContext& context, Data::Size size, bool addressable, bool is_volatile)
{
    m_impl_ptr = std::make_unique<Impl>(IBuffer::CreateConstantBuffer(context.GetInterface(), size, addressable, is_volatile));
    Transmitter<IObjectCallback>::Reset(&m_impl_ptr->GetInterface());
    Transmitter<IResourceCallback>::Reset(&m_impl_ptr->GetInterface());
}

void Buffer::InitReadBackBuffer(const RenderContext& context, Data::Size size)
{
    m_impl_ptr = std::make_unique<Impl>(IBuffer::CreateReadBackBuffer(context.GetInterface(), size));
    Transmitter<IObjectCallback>::Reset(&m_impl_ptr->GetInterface());
    Transmitter<IResourceCallback>::Reset(&m_impl_ptr->GetInterface());
}

void Buffer::Release()
{
    Transmitter<IObjectCallback>::Reset();
    Transmitter<IResourceCallback>::Reset();
    m_impl_ptr.release();
}

bool Buffer::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IBuffer& Buffer::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

bool Buffer::SetName(std::string_view name) const
{
    return GetPrivateImpl(m_impl_ptr).SetName(name);
}

std::string_view Buffer::GetName() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetName();
}

bool Buffer::SetState(State state) const
{
    return GetPrivateImpl(m_impl_ptr).SetState(state);
}

bool Buffer::SetState(State state, Barriers& out_barriers) const
{
    Ptr<IResourceBarriers> out_barriers_ptr = out_barriers.GetInterface().GetPtr();
    const bool state_changed = GetPrivateImpl(m_impl_ptr).SetState(state, out_barriers_ptr);
    if (!out_barriers.IsInitialized() && out_barriers_ptr)
    {
        out_barriers = ResourceBarriers(out_barriers_ptr);
    }
    return state_changed;
}

bool Buffer::SetOwnerQueueFamily(uint32_t family_index) const
{
    return GetPrivateImpl(m_impl_ptr).SetOwnerQueueFamily(family_index);
}

bool Buffer::SetOwnerQueueFamily(uint32_t family_index, Barriers& out_barriers) const
{
    Ptr<IResourceBarriers> out_barriers_ptr = out_barriers.GetInterface().GetPtr();
    const bool state_changed = GetPrivateImpl(m_impl_ptr).SetOwnerQueueFamily(family_index, out_barriers_ptr);
    if (!out_barriers.IsInitialized() && out_barriers_ptr)
    {
        out_barriers = ResourceBarriers(out_barriers_ptr);
    }
    return state_changed;
}

void Buffer::SetData(const SubResources& sub_resources, const CommandQueue& target_cmd_queue) const
{
    GetPrivateImpl(m_impl_ptr).SetData(sub_resources, target_cmd_queue.GetInterface());
}

void Buffer::RestoreDescriptorViews(const DescriptorByViewId& descriptor_by_view_id) const
{
    GetPrivateImpl(m_impl_ptr).RestoreDescriptorViews(descriptor_by_view_id);
}

SubResource Buffer::GetData(const SubResource::Index& sub_resource_index, const BytesRangeOpt& data_range) const
{
    return GetPrivateImpl(m_impl_ptr).GetData(sub_resource_index, data_range);
}

Data::Size Buffer::GetDataSize(Data::MemoryState size_type) const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetDataSize(size_type);
}

Data::Size Buffer::GetSubResourceDataSize(const SubResource::Index& sub_resource_index) const
{
    return GetPrivateImpl(m_impl_ptr).GetSubResourceDataSize(sub_resource_index);
}

const SubResource::Count& Buffer::GetSubresourceCount() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetSubresourceCount();
}

ResourceType Buffer::GetResourceType() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetResourceType();
}

ResourceState Buffer::GetState() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetState();
}

ResourceUsageMask Buffer::GetUsage() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetUsage();
}

const Buffer::DescriptorByViewId& Buffer::GetDescriptorByViewId() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetDescriptorByViewId();
}

const IContext& Buffer::GetContext() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetContext();
}

const Opt<uint32_t>& Buffer::GetOwnerQueueFamily() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetOwnerQueueFamily();
}

const Buffer::Settings& Buffer::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetSettings();
}

uint32_t Buffer::GetFormattedItemsCount() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetFormattedItemsCount();
}

static Refs<IBuffer> GetIBufferRefs(const Refs<Buffer>& buffer_refs)
{
    Refs<IBuffer> i_buffer_refs;
    for(const Ref<Buffer>& buffer_ref : buffer_refs)
    {
        i_buffer_refs.emplace_back(buffer_ref.get().GetInterface());
    }
    return i_buffer_refs;
}

class BufferSet::Impl
    : public ImplWrapper<IBufferSet, BufferSetImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(BufferSet);

BufferSet::BufferSet(UniquePtr<Impl>&& impl_ptr)
    : Transmitter(impl_ptr->GetInterface())
    , m_impl_ptr(std::move(impl_ptr))
{
}

BufferSet::BufferSet(const Ptr<IBufferSet>& interface_ptr)
    : BufferSet(std::make_unique<Impl>(interface_ptr))
{
}

BufferSet::BufferSet(IBufferSet& interface_ref)
    : BufferSet(std::dynamic_pointer_cast<IBufferSet>(interface_ref.GetDerivedPtr<IBufferSet>()))
{
}

BufferSet::BufferSet(BufferType buffers_type, const Refs<Buffer>& buffer_refs)
    : BufferSet(IBufferSet::Create(buffers_type, GetIBufferRefs(buffer_refs)))
{
}

void BufferSet::Init(BufferType buffers_type, const Refs<Buffer>& buffer_refs)
{
    m_impl_ptr = std::make_unique<Impl>(IBufferSet::Create(buffers_type, GetIBufferRefs(buffer_refs)));
    Transmitter::Reset(&m_impl_ptr->GetInterface());
}

void BufferSet::Release()
{
    Transmitter::Reset();
    m_impl_ptr.release();
}

bool BufferSet::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IBufferSet& BufferSet::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

BufferType BufferSet::GetType() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetType();
}

Data::Size BufferSet::GetCount() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetCount();
}

const BufferSet::Buffers& BufferSet::GetRefs() const noexcept
{
    const Refs<IBuffer>& i_buffer_refs = GetPrivateImpl(m_impl_ptr).GetRefs();
    if (m_buffers.size() == i_buffer_refs.size())
        return m_buffers;

    m_buffers.clear();
    for(const Ref<IBuffer>& i_buffer_ref : i_buffer_refs)
    {
        m_buffers.emplace_back(i_buffer_ref.get());
    }
    return m_buffers;
}

std::string BufferSet::GetNames() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetNames();
}

const Buffer& BufferSet::operator[](Data::Index index) const
{
    const Buffers& buffers = GetRefs();
    META_CHECK_ARG_LESS(index, buffers.size());
    return buffers[index];
}

} // namespace Methane::Graphics::Rhi
