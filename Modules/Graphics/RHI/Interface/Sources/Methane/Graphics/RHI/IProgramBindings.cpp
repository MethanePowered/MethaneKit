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

FILE: Methane/Graphics/RHI/IProgramBindings.cpp
Methane program bindings interface for resources binding to program arguments.

******************************************************************************/

#include <Methane/Graphics/RHI/IProgramBindings.h>

#include <Methane/Instrumentation.h>

#include <fmt/format.h>
#include <fmt/ranges.h>

template<>
struct fmt::formatter<Methane::Graphics::Rhi::ProgramArgument>
{
    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }

    template<typename FormatContext>
    auto format(const Methane::Graphics::Rhi::ProgramArgument& program_argument, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", static_cast<std::string>(program_argument));
    }
};

namespace Methane::Graphics::Rhi
{

ProgramArgumentConstantModificationException::ProgramArgumentConstantModificationException(const Rhi::IProgram::Argument& argument)
    : std::logic_error(fmt::format("Can not modify constant argument binding '{}' of {} shaders.",
                                   argument.GetName(), magic_enum::enum_name(argument.GetShaderType())))
{
    META_FUNCTION_TASK();
}

ProgramBindingsUnboundArgumentsException::ProgramBindingsUnboundArgumentsException(const Rhi::IProgram& program, const Rhi::IProgram::Arguments& unbound_arguments)
    : std::runtime_error(fmt::format("Some arguments of program '{}' are not bound to any resource:\n{}", program.GetName(), unbound_arguments))
    , m_program(program)
    , m_unbound_arguments(unbound_arguments)
{
    META_FUNCTION_TASK();
}

ProgramBindingsApplyBehavior::ProgramBindingsApplyBehavior() noexcept
    : mask(0U)
{
}

ProgramBindingsApplyBehavior::ProgramBindingsApplyBehavior(uint32_t mask) noexcept
    : mask(mask)
{
}

ProgramBindingsApplyBehavior::ProgramBindingsApplyBehavior(const std::initializer_list<Bit>& bits)
    : mask(0U)
{
    META_FUNCTION_TASK();
    for(Bit bit : bits)
    {
        SetBit(bit, true);
    }
}

bool ProgramBindingsApplyBehavior::operator==(const ProgramBindingsApplyBehavior& other) const noexcept
{
    return mask == other.mask;
}

bool ProgramBindingsApplyBehavior::operator!=(const ProgramBindingsApplyBehavior& other) const noexcept
{
    return mask != other.mask;
}

void ProgramBindingsApplyBehavior::SetBit(Bit bit, bool value)
{
    META_FUNCTION_TASK();
    switch(bit)
    {
    case Bit::ConstantOnce:    constant_once    = value; break;
    case Bit::ChangesOnly:     changes_only     = value; break;
    case Bit::StateBarriers:   state_barriers   = value; break;
    case Bit::RetainResources: retain_resources = value; break;
    default: META_UNEXPECTED_ARG(bit);
    }
}

bool ProgramBindingsApplyBehavior::HasBit(Bit bit) const
{
    META_FUNCTION_TASK();
    switch(bit)
    {
    case Bit::ConstantOnce:    return constant_once;
    case Bit::ChangesOnly:     return changes_only;
    case Bit::StateBarriers:   return state_barriers;
    case Bit::RetainResources: return retain_resources;
    default: META_UNEXPECTED_ARG(bit);
    }
}

std::vector<ProgramBindingsApplyBehavior::Bit> ProgramBindingsApplyBehavior::GetBits() const
{
    META_FUNCTION_TASK();
    std::vector<Bit> bits;
    if (constant_once)
        bits.push_back(Bit::ConstantOnce);
    if (changes_only)
        bits.push_back(Bit::ChangesOnly);
    if (state_barriers)
        bits.push_back(Bit::StateBarriers);
    if (retain_resources)
        bits.push_back(Bit::RetainResources);
    return bits;
}

std::vector<std::string> ProgramBindingsApplyBehavior::GetBitNames() const
{
    META_FUNCTION_TASK();
    const std::vector<Bit> bits = GetBits();
    std::vector<std::string> bit_names;
    for(Bit bit : bits)
    {
        bit_names.emplace_back(magic_enum::enum_name(bit));
    }
    return bit_names;
}

} // namespace Methane::Graphics::Rhi
