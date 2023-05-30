/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/IComputeState.cpp
Methane render state interface: specifies configuration of the graphics pipeline.

******************************************************************************/

#include <Methane/Graphics/RHI/IComputeState.h>
#include <Methane/Graphics/RHI/IContext.h>
#include <Methane/Graphics/RHI/IProgram.h>
#include <Methane/Graphics/TypeFormatters.hpp>

#include <Methane/Instrumentation.h>

#include <fmt/format.h>

namespace Methane::Graphics::Rhi
{

bool ComputeStateSettings::operator==(const ComputeStateSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(program_ptr) ==
           std::tie(other.program_ptr);
}

bool ComputeStateSettings::operator!=(const ComputeStateSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(program_ptr) !=
           std::tie(other.program_ptr);
}

ComputeStateSettings::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("  - Program '{}';\n"
                       "  - Thread Group Size: {}.",
                       program_ptr->GetName(),
                       thread_group_size);
}

Ptr<IComputeState> IComputeState::Create(const IContext& context, const Settings& state_settings)
{
    META_FUNCTION_TASK();
    return context.CreateComputeState(state_settings);
}

} // namespace Methane::Graphics::Rhi
