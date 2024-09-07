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
#include <Methane/Graphics/RHI/ICommandKit.h>
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

bool RootConstantAccessor::SetRootConstant(const Rhi::RootConstant& root_constant) const
{
    META_FUNCTION_TASK();
    if (root_constant == GetRootConstant())
        return false;

    m_buffer.get().SetRootConstant(*this, root_constant);
    return true;
}

const Rhi::ResourceView RootConstantAccessor::GetResourceView() const
{
    META_FUNCTION_TASK();
    return Rhi::ResourceView(m_buffer.get().GetBuffer(), m_buffer_range.GetStart(), m_buffer_range.GetLength());
}

RootConstantBuffer::RootConstantBuffer(Context& context)
    : m_context(context)
{
    META_FUNCTION_TASK();
    dynamic_cast<Data::IEmitter<IContextCallback>&>(context).Connect(*this);
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
    Accessor::Range buffer_range;
    if (m_free_ranges.IsEmpty())
    {
        m_deferred_size += root_constant_size;
        buffer_range = Accessor::Range(m_deferred_size - root_constant_size, m_deferred_size);
    }
    else
    {
        buffer_range = ReserveRange(m_free_ranges, root_constant_size);
    }
    return std::make_unique<Accessor>(*this, buffer_range);
}

void RootConstantBuffer::ReleaseRootConstant(const Accessor& reservation)
{
    META_FUNCTION_TASK();
    m_free_ranges.Add(reservation.GetBufferRange());
}

void RootConstantBuffer::SetRootConstant(const Accessor& accessor, const Rhi::RootConstant& root_constant)
{
    META_FUNCTION_TASK();
    Data::Bytes&           data       = GetData();
    const Accessor::Range& data_range = accessor.GetBufferRange();

    META_CHECK_ARG_EQUAL_DESCR(data_range.GetLength(), root_constant.GetDataSize(), "wrong root constant size");
    std::copy(root_constant.GetDataPtr(), root_constant.GetDataEndPtr(), data.data() + data_range.GetStart());

    m_buffer_data_changed = true;
    m_context.RequestDeferredAction(Rhi::ContextDeferredAction::CompleteInitialization);
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
    if (m_buffer_ptr && m_buffer_ptr->GetSettings().size >= m_deferred_size)
        return *m_buffer_ptr;

    const bool buffer_changed = !!m_buffer_ptr;
    const auto buffer_settings = Rhi::BufferSettings::ForConstantBuffer(m_deferred_size, true, false);
    m_buffer_ptr               = m_context.CreateBuffer(buffer_settings);
    m_buffer_ptr->SetName("Global Root Constants Buffer");
    m_buffer_data_changed = false;

    if (buffer_changed)
        Emit(&ICallback::OnRootConstantBufferChanged, *this);

    return *m_buffer_ptr;
}

void RootConstantBuffer::UpdateGpuBuffer(Rhi::ICommandQueue& target_cmd_queue)
{
    META_FUNCTION_TASK();
    if (!m_buffer_data_changed)
        return;

    META_CHECK_ARG_NOT_EMPTY(m_buffer_data);
    Rhi::IBuffer& buffer = GetBuffer();
    buffer.SetData(target_cmd_queue, Rhi::SubResource(m_buffer_data));
}

void RootConstantBuffer::OnContextCompletingInitialization(Rhi::IContext& context)
{
    META_FUNCTION_TASK();
    UpdateGpuBuffer(context.GetDefaultCommandKit(Rhi::CommandListType::Transfer).GetQueue());
}

void RootConstantBuffer::OnContextReleased(Rhi::IContext&)
{
    // intentionally unimplemented
}

void RootConstantBuffer::OnContextInitialized(Rhi::IContext&)
{
    // intentionally unimplemented
}

} // namespace Methane::Graphics::Base
