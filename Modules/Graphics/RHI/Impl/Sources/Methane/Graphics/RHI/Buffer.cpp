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

#ifdef META_GFX_METAL
#include <Buffer.hh>
#else
#include <Buffer.h>
#endif

#include "Pimpl.hpp"

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(Buffer);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(Buffer);

Buffer::Buffer(Ptr<Impl>&& impl_ptr)
    : m_impl_ptr(std::move(impl_ptr))
{
}

Buffer::Buffer(const Ptr<IBuffer>& interface_ptr)
    : Buffer(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

Buffer::Buffer(IBuffer& interface_ref)
    : Buffer(interface_ref.GetDerivedPtr<IBuffer>())
{
}

void Buffer::InitVertexBuffer(const IContext& context, Data::Size size, Data::Size stride, bool is_volatile)
{
    m_impl_ptr = std::dynamic_pointer_cast<Impl>(IBuffer::CreateVertexBuffer(context, size, stride, is_volatile));
}

void Buffer::InitIndexBuffer(const IContext& context, Data::Size size, PixelFormat format, bool is_volatile)
{
    m_impl_ptr = std::dynamic_pointer_cast<Impl>(IBuffer::CreateIndexBuffer(context, size, format, is_volatile));
}

void Buffer::InitConstantBuffer(const IContext& context, Data::Size size, bool addressable, bool is_volatile)
{
    m_impl_ptr = std::dynamic_pointer_cast<Impl>(IBuffer::CreateConstantBuffer(context, size, addressable, is_volatile));
}

void Buffer::InitReadBackBuffer(const IContext& context, Data::Size size)
{
    m_impl_ptr = std::dynamic_pointer_cast<Impl>(IBuffer::CreateReadBackBuffer(context, size));
}

void Buffer::Release()
{
    m_impl_ptr.reset();
}

bool Buffer::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IBuffer& Buffer::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<IBuffer> Buffer::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool Buffer::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view Buffer::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void Buffer::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void Buffer::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

bool Buffer::SetState(State state) const
{
    return GetImpl(m_impl_ptr).SetState(state);
}

bool Buffer::SetState(State state, Barriers& out_barriers) const
{
    Ptr<IResourceBarriers> out_barriers_ptr = out_barriers.GetInterfacePtr();
    const bool state_changed = GetImpl(m_impl_ptr).SetState(state, out_barriers_ptr);
    if (!out_barriers.IsInitialized() && out_barriers_ptr)
    {
        out_barriers = ResourceBarriers(out_barriers_ptr);
    }
    return state_changed;
}

bool Buffer::SetOwnerQueueFamily(uint32_t family_index) const
{
    return GetImpl(m_impl_ptr).SetOwnerQueueFamily(family_index);
}

bool Buffer::SetOwnerQueueFamily(uint32_t family_index, Barriers& out_barriers) const
{
    Ptr<IResourceBarriers> out_barriers_ptr = out_barriers.GetInterfacePtr();
    const bool state_changed = GetImpl(m_impl_ptr).SetOwnerQueueFamily(family_index, out_barriers_ptr);
    if (!out_barriers.IsInitialized() && out_barriers_ptr)
    {
        out_barriers = ResourceBarriers(out_barriers_ptr);
    }
    return state_changed;
}

void Buffer::SetData(const SubResources& sub_resources, const CommandQueue& target_cmd_queue) const
{
    GetImpl(m_impl_ptr).SetData(sub_resources, target_cmd_queue.GetInterface());
}

void Buffer::RestoreDescriptorViews(const DescriptorByViewId& descriptor_by_view_id) const
{
    GetImpl(m_impl_ptr).RestoreDescriptorViews(descriptor_by_view_id);
}

SubResource Buffer::GetData(const SubResource::Index& sub_resource_index, const BytesRangeOpt& data_range) const
{
    return GetImpl(m_impl_ptr).GetData(sub_resource_index, data_range);
}

Data::Size Buffer::GetDataSize(Data::MemoryState size_type) const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetDataSize(size_type);
}

Data::Size Buffer::GetSubResourceDataSize(const SubResource::Index& sub_resource_index) const
{
    return GetImpl(m_impl_ptr).GetSubResourceDataSize(sub_resource_index);
}

const SubResource::Count& Buffer::GetSubresourceCount() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetSubresourceCount();
}

ResourceType Buffer::GetResourceType() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetResourceType();
}

ResourceState Buffer::GetState() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetState();
}

ResourceUsageMask Buffer::GetUsage() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetUsage();
}

const Buffer::DescriptorByViewId& Buffer::GetDescriptorByViewId() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetDescriptorByViewId();
}

const IContext& Buffer::GetContext() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetContext();
}

const Opt<uint32_t>& Buffer::GetOwnerQueueFamily() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetOwnerQueueFamily();
}

void Buffer::Connect(Data::Receiver<IResourceCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IResourceCallback>::Connect(receiver);
}

void Buffer::Disconnect(Data::Receiver<IResourceCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IResourceCallback>::Disconnect(receiver);
}

const Buffer::Settings& Buffer::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetSettings();
}

uint32_t Buffer::GetFormattedItemsCount() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetFormattedItemsCount();
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

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(BufferSet);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(BufferSet);

BufferSet::BufferSet(Ptr<Impl>&& impl_ptr)
    : m_impl_ptr(std::move(impl_ptr))
{
}

BufferSet::BufferSet(const Ptr<IBufferSet>& interface_ptr)
    : BufferSet(std::dynamic_pointer_cast<Impl>(interface_ptr))
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
    m_buffers.clear();
    m_impl_ptr = std::dynamic_pointer_cast<Impl>(IBufferSet::Create(buffers_type, GetIBufferRefs(buffer_refs)));
}

void BufferSet::Release()
{
    m_impl_ptr.reset();
    m_buffers.clear();
}

bool BufferSet::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IBufferSet& BufferSet::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<IBufferSet> BufferSet::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool BufferSet::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view BufferSet::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void BufferSet::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void BufferSet::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

BufferType BufferSet::GetType() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetType();
}

Data::Size BufferSet::GetCount() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetCount();
}

const BufferSet::Buffers& BufferSet::GetRefs() const noexcept
{
    const Refs<IBuffer>& i_buffer_refs = GetImpl(m_impl_ptr).GetRefs();
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
    return GetImpl(m_impl_ptr).GetNames();
}

const Buffer& BufferSet::operator[](Data::Index index) const
{
    const Buffers& buffers = GetRefs();
    META_CHECK_ARG_LESS(index, buffers.size());
    return buffers[index];
}

} // namespace Methane::Graphics::Rhi
