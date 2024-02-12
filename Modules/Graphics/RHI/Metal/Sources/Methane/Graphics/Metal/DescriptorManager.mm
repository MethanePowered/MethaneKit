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
    Data::Size cummulative_argument_buffer_size = 0U;
    ForEachProgramBinding([&cummulative_argument_buffer_size](Rhi::IProgramBindings& program_bindings)
    {
        const Data::Size argument_buffer_size = static_cast<Program&>(program_bindings.GetProgram()).GetArgumentBuffersSize();
        const ArgumentsRange arguments_range(cummulative_argument_buffer_size, argument_buffer_size);
        static_cast<ProgramBindings&>(program_bindings).CompleteInitialization(arguments_range);
        cummulative_argument_buffer_size += argument_buffer_size;
    });
}

void DescriptorManager::Release()
{
    Base::DescriptorManager::Release();
    m_argument_buffer.reset();
    m_free_argument_ranges.Clear();
}

} // namespace Methane::Graphics::Metal
