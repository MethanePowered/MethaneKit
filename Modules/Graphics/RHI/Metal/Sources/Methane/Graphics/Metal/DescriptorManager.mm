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

DescriptorManager::DescriptorManager(Base::Context& context)
    : Base::DescriptorManager(context)
{
}

void DescriptorManager::UpdateProgramBindings(ProgramBindings& program_bindings)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_argument_buffer_mutex);
    program_bindings.CompleteInitialization(m_argument_buffer_data);
    GetContext().RequestDeferredAction(Rhi::ContextDeferredAction::CompleteInitialization);
}

void DescriptorManager::AddProgramBindings(Rhi::IProgramBindings& program_bindings)
{
    META_FUNCTION_TASK();
    auto& metal_program_bindings = dynamic_cast<ProgramBindings&>(program_bindings);
    const Data::Size arguments_range_size = static_cast<Program&>(program_bindings.GetProgram()).GetArgumentBuffersSize();
    if (!arguments_range_size)
        return;

    std::scoped_lock lock_guard(m_argument_buffer_mutex);
    const ArgumentsRange arguments_range = ReserveArgumentsRange(arguments_range_size);
    metal_program_bindings.CompleteInitialization(m_argument_buffer_data, arguments_range);
    GetContext().RequestDeferredAction(Rhi::ContextDeferredAction::CompleteInitialization);
}

void DescriptorManager::RemoveProgramBindings(Rhi::IProgramBindings& program_bindings)
{
    META_FUNCTION_TASK();
    auto& metal_program_bindings = dynamic_cast<ProgramBindings&>(program_bindings);
    ReleaseArgumentsRange(metal_program_bindings.GetArgumentsRange());
}

void DescriptorManager::CompleteInitialization()
{
    // Argument buffer initialization is done in OnContextCompletingInitialization callback,
    // so that buffer data is uploaded to the GPU in UploadResources() in Base::Context::CompleteInitialization()
}

void DescriptorManager::OnContextCompletingInitialization(Rhi::IContext&)
{
    META_FUNCTION_TASK();
    META_LOG("Metal Descriptor Manager is completing initialization of the global argument buffer...");
    std::scoped_lock lock_guard(m_argument_buffer_mutex);

    if (m_argument_buffer_data.empty())
    {
        m_argument_buffer_ptr.reset();
        return;
    }

    if (!m_argument_buffer_ptr || m_argument_buffer_data.size() > m_argument_buffer_ptr->GetSettings().size)
    {
        Rhi::BufferSettings argument_buffer_settings
        {
            Rhi::BufferType::Constant,
            Rhi::ResourceUsageMask(Rhi::ResourceUsage::ShaderRead),
            static_cast<Data::Size>(m_argument_buffer_data.size()),
            0U,
            PixelFormat::Unknown,
            Rhi::BufferStorageMode::Managed
        };
        m_argument_buffer_ptr = GetContext().CreateBuffer(argument_buffer_settings);
        m_argument_buffer_ptr->SetName("Global Argument Buffer");
    }

    // Compute queue is used as a target command queue for the argument buffer data transfer,
    // because in Metal it queue ownership does not matter.
    Rhi::ICommandQueue& compute_queue = GetContext().GetDefaultCommandKit(Rhi::CommandListType::Compute).GetQueue();
    m_argument_buffer_ptr->SetData(compute_queue, Rhi::SubResource(m_argument_buffer_data));
}

void DescriptorManager::Release()
{
    Base::DescriptorManager::Release();

    std::scoped_lock lock_guard(m_argument_buffer_mutex);
    m_argument_buffer_ptr.reset();
}

DescriptorManager::ArgumentsRange DescriptorManager::ReserveArgumentsRange(Data::Size range_size)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_ZERO_DESCR(range_size, "unable to reserve empty arguments range");

    if (const ArgumentsRange reserved_range = Data::ReserveRange(m_argument_buffer_free_ranges, range_size);
        reserved_range)
        return reserved_range;

    const ArgumentsRange arguments_range(m_argument_buffer_data.size(), m_argument_buffer_data.size() + range_size);
    m_argument_buffer_data.resize(m_argument_buffer_data.size() + range_size);
    return arguments_range;
}

void DescriptorManager::ReleaseArgumentsRange(const ArgumentsRange& range)
{
    META_FUNCTION_TASK();
    if (range.IsEmpty())
        return;

    std::scoped_lock lock_guard(m_argument_buffer_mutex);
    m_argument_buffer_free_ranges.Add(range);
}

} // namespace Methane::Graphics::Metal
