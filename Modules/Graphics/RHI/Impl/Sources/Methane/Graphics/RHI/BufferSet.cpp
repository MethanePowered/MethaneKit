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

#include <Methane/Graphics/RHI/BufferSet.h>
#include <Methane/Graphics/RHI/Buffer.h>

#ifdef META_GFX_METAL
#include <BufferSet.hh>
#else
#include <BufferSet.h>
#endif

#include <Methane/Pimpl.hpp>

namespace Methane::Graphics::Rhi
{

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

BufferSet::BufferSet(const Ptr<IBufferSet>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
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
    META_CHECK_LESS(index, buffers.size());
    return buffers[index];
}

} // namespace Methane::Graphics::Rhi
