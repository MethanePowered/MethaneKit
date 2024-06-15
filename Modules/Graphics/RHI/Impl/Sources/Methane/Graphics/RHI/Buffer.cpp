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
#include <Methane/Graphics/RHI/ResourceBarriers.h>
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/RenderContext.h>

#ifdef META_GFX_METAL
#include <Buffer.hh>
#else
#include <Buffer.h>
#endif

#include <Methane/Pimpl.hpp>

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(Buffer);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(Buffer);

Buffer::Buffer(const Ptr<IBuffer>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

Buffer::Buffer(IBuffer& interface_ref)
    : Buffer(interface_ref.GetDerivedPtr<IBuffer>())
{
}

Buffer::Buffer(const IContext& context, const BufferSettings& settings)
    : Buffer(IBuffer::Create(context, settings))
{
}

Buffer::Buffer(const RenderContext& context, const BufferSettings& settings)
    : Buffer(context.GetInterface(), settings)
{
}

Buffer::Buffer(const ComputeContext& context, const BufferSettings& settings)
    : Buffer(context.GetInterface(), settings)
{
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

void Buffer::SetData(const CommandQueue& target_cmd_queue, const SubResource& sub_resource) const
{
    GetImpl(m_impl_ptr).SetData(target_cmd_queue.GetInterface(), sub_resource);
}

void Buffer::RestoreDescriptorViews(const DescriptorByViewId& descriptor_by_view_id) const
{
    GetImpl(m_impl_ptr).RestoreDescriptorViews(descriptor_by_view_id);
}

SubResource Buffer::GetData(const Rhi::CommandQueue& target_cmd_queue, const BytesRangeOpt& data_range) const
{
    return GetImpl(m_impl_ptr).GetData(target_cmd_queue.GetInterface(), data_range);
}

Data::Size Buffer::GetDataSize(Data::MemoryState size_type) const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetDataSize(size_type);
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

Rhi::ResourceView Buffer::GetBufferView(Data::Size offset, Data::Size size) const
{
    return GetImpl(m_impl_ptr).GetBufferView(offset, size);
}

Rhi::ResourceView Buffer::GetResourceView() const
{
    return GetImpl(m_impl_ptr).GetResourceView();
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

} // namespace Methane::Graphics::Rhi
