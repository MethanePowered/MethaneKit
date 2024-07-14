/******************************************************************************

Copyright 2024 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Base/RootConstantBuffer.h
Root constant buffer used for sub-allocations for small constants buffer views,
bound to Program using ProgramArgumentBinging as RootConstant.

******************************************************************************/

#include <Methane/Graphics/Base/RootConstantBuffer.h>
#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/Base/Buffer.h>
#include <Methane/Data/RangeUtils.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Base
{

RootConstantAccessor::RootConstantAccessor(RootConstantBuffer& buffer, const Range& buffer_range)
    : m_buffer(buffer)
    , m_buffer_range(buffer_range)
{
}

RootConstantAccessor::~RootConstantAccessor()
{
    META_FUNCTION_TASK();
    m_buffer.get().ReleaseRootConstant(*this);
}

Rhi::RootConstant RootConstantAccessor::GetRootConstant() const
{
    META_FUNCTION_TASK();
    return Rhi::RootConstant(m_buffer.get().GetData().data(), m_buffer_range.GetLength());
}

void RootConstantAccessor::SetRootConstant(const Rhi::RootConstant& root_constant)
{
    META_FUNCTION_TASK();
    m_buffer.get().SetRootConstant(*this, root_constant);
}

RootConstantBuffer::RootConstantBuffer(const Context& context)
    : m_context(context)
{
}

RootConstantBuffer::~RootConstantBuffer()
{
    META_FUNCTION_TASK();
    assert((!m_deferred_size && m_free_ranges.IsEmpty()) ||
           m_free_ranges == RangeSet({ { 0, m_deferred_size } }));
}

UniquePtr<RootConstantAccessor> RootConstantBuffer::ReserveRootConstant(Data::Size root_constant_size)
{
    META_FUNCTION_TASK();
    if (m_free_ranges.IsEmpty())
    {
        m_deferred_size += root_constant_size;
        return std::make_unique<Accessor>(*this, Accessor::Range(m_deferred_size - root_constant_size, m_deferred_size));
    }
    return std::make_unique<Accessor>(*this, Data::ReserveRange(m_free_ranges, root_constant_size));
}

void RootConstantBuffer::ReleaseRootConstant(const Accessor& reservation)
{
    META_FUNCTION_TASK();
    const Data::Size reserved_size = reservation.GetBufferRange().GetLength();
    m_free_ranges.Add(reservation.GetBufferRange());
    if (m_deferred_size > reserved_size)
    {
        m_deferred_size -= reserved_size;
    }
}

void RootConstantBuffer::SetRootConstant(const Accessor& accessor, const Rhi::RootConstant& root_constant)
{
    META_FUNCTION_TASK();
    Data::Bytes& data = GetData();
    const Accessor::Range& data_range = accessor.GetBufferRange();
    META_CHECK_ARG_EQUAL_DESCR(data_range.GetLength(), root_constant.GetDataSize(), "root constant size is unexpected");
    std::copy(root_constant.GetDataPtr(), root_constant.GetDataEndPtr(), data.data() + data_range.GetStart());
}

Data::Bytes& RootConstantBuffer::GetData()
{
    META_FUNCTION_TASK();
    if (static_cast<Data::Size>(m_buffer_data.size()) != m_deferred_size)
    {
        m_buffer_data.resize(m_deferred_size);
    }
    return m_buffer_data;
}

Rhi::IBuffer& RootConstantBuffer::GetBuffer()
{
    META_FUNCTION_TASK();
    if (m_buffer_ptr || m_buffer_ptr->GetSettings().size >= m_deferred_size)
        return *m_buffer_ptr;

    const auto buffer_settings = Rhi::BufferSettings::ForConstantBuffer(m_deferred_size, true, false);
    m_buffer_ptr = m_context.CreateBuffer(buffer_settings);
    return *m_buffer_ptr;
}

} // namespace Methane::Graphics::Base
