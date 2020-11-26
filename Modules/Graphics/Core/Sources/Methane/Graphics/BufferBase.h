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

FILE: Methane/Graphics/BufferBase.h
Base implementation of the buffer interface.

******************************************************************************/

#pragma once

#include "ResourceBase.h"

#include <Methane/Graphics/Buffer.h>

namespace Methane::Graphics
{

class BufferBase
    : public Buffer
    , public ResourceBase
{
public:
    BufferBase(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage = DescriptorByUsage());

    // Resource interface
    Data::Size GetDataSize(Data::MemoryState size_type = Data::MemoryState::Reserved) const noexcept override;

    // Buffer interface
    const Settings& GetSettings() const noexcept final       { return m_settings; }
    uint32_t GetFormattedItemsCount() const noexcept final;

    Ptr<BufferBase> GetBufferPtr()                           { return std::static_pointer_cast<BufferBase>(GetBasePtr()); }

private:
    Settings    m_settings;
};

class BufferSetBase
    : public BufferSet
    , public ObjectBase
{
public:
    BufferSetBase(Buffer::Type buffers_type, const Refs<Buffer>& buffer_refs);

    // Buffers interface
    Buffer::Type        GetType() const noexcept final  { return m_buffers_type; }
    Data::Size          GetCount() const noexcept final { return static_cast<Data::Size>(m_refs.size()); }
    const Refs<Buffer>& GetRefs() const noexcept final  { return m_refs; }
    Buffer&             operator[](Data::Index index) const final;

    const RawPtrs<BufferBase>& GetRawPtrs() const noexcept { return m_raw_ptrs; }

private:
    const Buffer::Type  m_buffers_type;
    Refs<Buffer>        m_refs;
    Ptrs<Buffer>        m_ptrs;
    RawPtrs<BufferBase> m_raw_ptrs;
};

} // namespace Methane::Graphics
