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

FILE: Methane/Graphics/RHI/IProgram.cpp
Methane program interface: represents a collection of shaders set on graphics 
pipeline via state object and used to create resource binding objects.

******************************************************************************/

#include <Methane/Graphics/RHI/IProgram.h>
#include <Methane/Graphics/RHI/IContext.h>

#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>

namespace Methane::Graphics::Rhi
{

static const std::hash<std::string_view> g_argument_name_hash;

ProgramArgument::ProgramArgument(ShaderType shader_type, std::string_view argument_name) noexcept
    : m_shader_type(shader_type)
    , m_name(argument_name)
    , m_hash(g_argument_name_hash(m_name) ^ (magic_enum::enum_index(shader_type).value() << 1))
{ }

bool ProgramArgument::operator==(const ProgramArgument& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_hash, m_shader_type, m_name) ==
           std::tie(other.m_hash, other.m_shader_type, other.m_name);
}

bool ProgramArgument::operator<(const ProgramArgument& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_hash, m_shader_type, m_name) <
           std::tie(other.m_hash, other.m_shader_type, other.m_name);
}

ProgramArgument::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    return fmt::format("{} shaders argument '{}'", magic_enum::enum_name(m_shader_type), m_name);
}

ProgramArgumentAccessor::ProgramArgumentAccessor(ShaderType shader_type, std::string_view argument_name, Type accessor_type, bool addressable) noexcept
    : ProgramArgument(shader_type, argument_name)
    , m_accessor_type(accessor_type)
    , m_addressable(addressable)
{ }

ProgramArgumentAccessor::ProgramArgumentAccessor(const ProgramArgument& argument, Type accessor_type, bool addressable) noexcept
    : ProgramArgument(argument)
    , m_accessor_type(accessor_type)
    , m_addressable(addressable)
{ }

size_t ProgramArgumentAccessor::GetAccessorIndex() const noexcept
{
    return magic_enum::enum_index(m_accessor_type).value();
}

ProgramArgumentAccessor::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    return fmt::format("{} ({}{})", ProgramArgument::operator std::string(), magic_enum::enum_name(m_accessor_type), (m_addressable ? ", Addressable" : ""));
}

ProgramArgumentAccessors::const_iterator IProgram::FindArgumentAccessor(const ArgumentAccessors& argument_accessors, const ProgramArgument& argument)
{
    META_FUNCTION_TASK();
    if (const auto argument_desc_it = argument_accessors.find(argument);
        argument_desc_it != argument_accessors.end())
        return argument_desc_it;

    const Argument all_shaders_argument(ShaderType::All, argument.GetName());
    return argument_accessors.find(all_shaders_argument);
}

ProgramArgumentNotFoundException::ProgramArgumentNotFoundException(const IProgram& program, const ProgramArgument& argument)
    : std::invalid_argument(fmt::format("Program '{}' does not have argument '{}' of {} shader.",
                                        program.GetName(), argument.GetName(), magic_enum::enum_name(argument.GetShaderType())))
    , m_program(program)
    , m_argument(argument)
{ }

Ptr<IProgram> IProgram::Create(const IContext& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return context.CreateProgram(settings);
}

} // namespace Methane::Graphics::Rhi
