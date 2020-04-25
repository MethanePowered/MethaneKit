/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/BufferBase.cpp
Base implementation of the buffer interface.

******************************************************************************/

#include "BufferBase.h"
#include "DescriptorHeap.h"
#include "ContextBase.h"

#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

BufferBase::BufferBase(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : ResourceNT(Resource::Type::Buffer, settings.usage_mask, context, descriptor_by_usage)
    , m_settings(settings)
{
    if (!m_settings.size)
    {
        throw std::invalid_argument("Can not create buffer of zero size.");
    }
    META_FUNCTION_TASK();
}

Data::Size BufferBase::GetDataSize(Data::MemoryState size_type) const noexcept
{
    META_FUNCTION_TASK();
    return size_type == Data::MemoryState::Reserved ? m_settings.size : GetInitializedDataSize();
}

uint32_t BufferBase::GetFormattedItemsCount() const noexcept
{
    META_FUNCTION_TASK();
    return m_settings.item_stride_size > 0u ? GetDataSize(Data::MemoryState::Initialized) / m_settings.item_stride_size : 0u;
}

Ptr<BufferBase> BufferBase::GetPtr()
{
    META_FUNCTION_TASK();
    return std::dynamic_pointer_cast<BufferBase>(shared_from_this());
}

std::string Buffer::GetBufferTypeName(Type type) noexcept
{
    META_FUNCTION_TASK();
    switch (type)
    {
    case Type::Data:     return "Data";
    case Type::Index:    return "Index";
    case Type::Vertex:   return "Vertex";
    case Type::Constant: return "Constant";
    default:             assert(0);
    }
    return "Unknown";
}

BuffersBase::BuffersBase(Buffer::Type buffers_type, Refs<Buffer> buffer_refs)
    : m_buffers_type(buffers_type)
    , m_refs(std::move(buffer_refs))
{
    META_FUNCTION_TASK();
    if (m_refs.empty())
    {
        throw std::invalid_argument("Creating of empty buffers collection is not allowed.");
    }

    m_ptrs.reserve(m_refs.size());
    m_raw_ptrs.reserve(m_refs.size());
    for(const Ref<Buffer>& buffer_ref : m_refs)
    {
        if (buffer_ref.get().GetSettings().type != m_buffers_type)
        {
            std::invalid_argument("All buffers must be of the same type \"" + Buffer::GetBufferTypeName(m_buffers_type) + "\"");
        }
        BufferBase& buffer_base = static_cast<BufferBase&>(buffer_ref.get());
        m_ptrs.emplace_back(buffer_base.GetPtr());
        m_raw_ptrs.emplace_back(std::addressof(buffer_base));
    }
}

Buffer& BuffersBase::operator[](Data::Index index) const
{
    META_FUNCTION_TASK();
    if (index > m_refs.size())
        throw std::out_of_range("Buffer index " + std::to_string(index) +
                                " is out of buffers collection range (size = " + std::to_string(m_refs.size()) + ").");

    return m_refs[index].get();
}

} // namespace Methane::Graphics