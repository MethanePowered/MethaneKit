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

FILE: Methane/Graphics/Metal/DescriptorManager.mm
Metal descriptor manager of the argument buffer

******************************************************************************/

#include <Methane/Graphics/Metal/DescriptorManager.hh>
#include <Methane/Graphics/Metal/Buffer.hh>
#include <Methane/Graphics/Metal/Program.hh>
#include <Methane/Graphics/Metal/ProgramBindings.hh>
#include <Methane/Data/RangeUtils.hpp>

namespace Methane::Graphics::Metal
{

DescriptorManager::ArgumentsBuffer::DataLock::DataLock(ArgumentsBuffer& arg_buffer)
    : m_lock(arg_buffer.m_mutex)
    , m_data_ptr(arg_buffer.GetDataPtr())
{
    META_FUNCTION_TASK();
}

DescriptorManager::ArgumentsBuffer::ArgumentsBuffer(const Base::Context& context, Rhi::ProgramArgumentAccessType access_type)
    : m_context(context)
    , m_access_type(access_type)
{
    META_FUNCTION_TASK();
}

const Rhi::IBuffer* DescriptorManager::ArgumentsBuffer::GetBuffer() const
{
    META_FUNCTION_TASK();
    if (m_data.empty())
    {
        m_buffer_ptr.reset();
    }
    else if (!m_buffer_ptr || m_data.size() > m_buffer_ptr->GetSettings().size)
    {
        CreateBuffer();
    }
    return m_buffer_ptr.get();
}

Rhi::IBuffer* DescriptorManager::ArgumentsBuffer::GetBuffer()
{
    META_FUNCTION_TASK();
    if (m_data.empty())
    {
        m_buffer_ptr.reset();
    }
    else if (!m_buffer_ptr || m_data.size() > m_buffer_ptr->GetSettings().size)
    {
        CreateBuffer();
    }
    return m_buffer_ptr.get();
}

DescriptorManager::ArgumentsRange DescriptorManager::ArgumentsBuffer::ReserveRange(Data::Size range_size)
{
    META_FUNCTION_TASK();
    META_CHECK_NOT_ZERO_DESCR(range_size, "unable to reserve empty arguments range");
    std::scoped_lock lock_guard(m_mutex);

    if (const ArgumentsRange reserved_range = Data::ReserveRange(m_free_ranges, range_size))
        return reserved_range;

    const ArgumentsRange arguments_range(m_data.size(), m_data.size() + range_size);
    m_data.resize(m_data.size() + range_size);
    return arguments_range;
}

void DescriptorManager::ArgumentsBuffer::ReleaseRange(const ArgumentsRange& range)
{
    META_FUNCTION_TASK();
    if (range.IsEmpty())
        return;

    std::scoped_lock lock_guard(m_mutex);
    m_free_ranges.Add(range);
}

void DescriptorManager::ArgumentsBuffer::Update()
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_mutex);

    Rhi::IBuffer* buffer_ptr = GetBuffer();
    if (!buffer_ptr)
        return;

    // Compute queue is used as a target command queue for the argument buffer data transfer,
    // because in Metal queue ownership does not matter.
    Rhi::ICommandQueue& compute_queue = m_context.GetDefaultCommandKit(Rhi::CommandListType::Compute).GetQueue();
    buffer_ptr->SetData(compute_queue, Rhi::SubResource(m_data));
}

void DescriptorManager::ArgumentsBuffer::CreateBuffer() const
{
    META_FUNCTION_TASK();
    m_buffer_ptr = m_context.CreateBuffer(
        Rhi::BufferSettings
            {
                Rhi::BufferType::Constant,
                Rhi::ResourceUsageMask(Rhi::ResourceUsage::ShaderRead),
                static_cast<Data::Size>(m_data.size()),
                0U,
                PixelFormat::Unknown,
                Rhi::BufferStorageMode::Managed
            });
    m_buffer_ptr->SetName(fmt::format("{} Argument Buffer", magic_enum::enum_name(m_access_type)));
}

void DescriptorManager::ArgumentsBuffer::Release()
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_mutex);
    m_buffer_ptr.reset();
}

DescriptorManager::DescriptorManager(Base::Context& context)
    : Base::DescriptorManager(context, false)
    , m_arguments_buffer_by_access_type{{ // Order by value of access type
        { /* 0: */ context, Rhi::ProgramArgumentAccessType::Constant },
        { /* 1: */ context, Rhi::ProgramArgumentAccessType::FrameConstant },
        { /* 2: */ context, Rhi::ProgramArgumentAccessType::Mutable },
    }}
{
}

void DescriptorManager::AddProgramBindings(Rhi::IProgramBindings& program_bindings)
{
    META_FUNCTION_TASK();
    Base::DescriptorManager::AddProgramBindings(program_bindings);

    auto& metal_program_bindings = dynamic_cast<ProgramBindings&>(program_bindings);
    auto& metal_program = static_cast<Program&>(program_bindings.GetProgram());
    const Data::Size mutable_args_range_size = metal_program.GetArgumentsBufferRangeSize(Rhi::ProgramArgumentAccessType::Mutable);
    if (!mutable_args_range_size)
        return;

    ArgumentsBuffer& mutable_args_buffer = GetArgumentsBuffer(Rhi::ProgramArgumentAccessType::Mutable);
    metal_program_bindings.SetMutableArgumentsRange(mutable_args_buffer.ReserveRange(mutable_args_range_size));
    GetContext().RequestDeferredAction(Rhi::ContextDeferredAction::CompleteInitialization);
}

void DescriptorManager::RemoveProgramBindings(Rhi::IProgramBindings& program_bindings)
{
    META_FUNCTION_TASK();
    Base::DescriptorManager::RemoveProgramBindings(program_bindings);

    auto& metal_program_bindings = dynamic_cast<ProgramBindings&>(program_bindings);
    const ArgumentsRange& mutable_args_range = metal_program_bindings.GetMutableArgumentsRange();
    if (!mutable_args_range.IsEmpty())
    {
        GetArgumentsBuffer(Rhi::ProgramArgumentAccessType::Mutable).ReleaseRange(mutable_args_range);
    }
}

void DescriptorManager::OnContextUploadingResources(Rhi::IContext&)
{
    META_FUNCTION_TASK();
    META_LOG("Metal Descriptor Manager is completing initialization of the global argument buffer...");

    // Complete initialization of program bindings before resources upload in Base::Context::CompleteInitialization()
    Base::DescriptorManager::CompleteInitialization();

    // Update argument buffers on the GPU to be uploaded right after this callback in Base::Context::CompleteInitialization()
    for(ArgumentsBuffer& arg_buffer : m_arguments_buffer_by_access_type)
    {
        arg_buffer.Update();
    }
}

void DescriptorManager::Release()
{
    META_FUNCTION_TASK();
    Base::DescriptorManager::Release();

    for(ArgumentsBuffer& arg_buffer : m_arguments_buffer_by_access_type)
    {
        arg_buffer.Release();
    }
}

const DescriptorManager::ArgumentsBuffer& DescriptorManager::GetArgumentsBuffer(Rhi::ProgramArgumentAccessType access_type) const noexcept
{
    return m_arguments_buffer_by_access_type[static_cast<uint32_t>(access_type)];
}

DescriptorManager::ArgumentsBuffer& DescriptorManager::GetArgumentsBuffer(Rhi::ProgramArgumentAccessType access_type) noexcept
{
    return m_arguments_buffer_by_access_type[static_cast<uint32_t>(access_type)];
}

} // namespace Methane::Graphics::Metal
