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

namespace Methane::Graphics::Metal
{

DescriptorManager::DescriptorManager(Base::Context& context)
    : Base::DescriptorManager(context)
{
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
    Base::DescriptorManager::ReleaseExpiredProgramBindings();

    // Fill argument buffer and send its data to GPU before UploadResources() in Base::Context::CompleteInitialization()
    Data::Size total_argument_buffer_size = 0U;
    Data::Bytes argument_buffer_data;
    ForEachProgramBinding([&total_argument_buffer_size, &argument_buffer_data](Rhi::IProgramBindings& program_bindings)
    {
        const Data::Size argument_buffer_size = static_cast<Program&>(program_bindings.GetProgram()).GetArgumentBuffersSize();
        const ArgumentsRange arguments_range(total_argument_buffer_size, total_argument_buffer_size + argument_buffer_size);
        total_argument_buffer_size += argument_buffer_size;
        argument_buffer_data.resize(total_argument_buffer_size);
        static_cast<ProgramBindings&>(program_bindings).CompleteInitialization(argument_buffer_data, arguments_range);
    });

    if (!total_argument_buffer_size)
    {
        m_argument_buffer_ptr.reset();
        return;
    }

    if (!m_argument_buffer_ptr || total_argument_buffer_size > m_argument_buffer_ptr->GetSettings().size)
    {
        m_argument_buffer_ptr = GetContext().CreateBuffer(Rhi::BufferSettings::ForConstantBuffer(total_argument_buffer_size));
        m_argument_buffer_ptr->SetName("Global Argument Buffer");
    }

    UpdateArgumentBufferData(std::move(argument_buffer_data));
}

void DescriptorManager::Release()
{
    Base::DescriptorManager::Release();
    m_argument_buffer_ptr.reset();
}

void DescriptorManager::UpdateArgumentBufferData(Data::Bytes&& argument_buffer_data)
{
    META_FUNCTION_TASK();
    if (argument_buffer_data.empty())
        return;

    // Compute queue is used as a target command queue for the argument buffer data transfer,
    // because in Metal it queue ownership does not matter.
    Rhi::ICommandQueue& compute_queue = GetContext().GetDefaultCommandKit(Rhi::CommandListType::Compute).GetQueue();
    m_argument_buffer_ptr->SetData(compute_queue, Rhi::SubResource(std::move(argument_buffer_data)));
}

} // namespace Methane::Graphics::Metal
