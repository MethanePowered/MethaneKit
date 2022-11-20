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

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

static const std::hash<std::string_view> g_argument_name_hash;

ProgramArgument::ProgramArgument(ShaderType shader_type, std::string_view argument_name) noexcept
    : m_shader_type(shader_type)
    , m_name(argument_name)
    , m_hash(g_argument_name_hash(m_name) ^ (magic_enum::enum_index(shader_type).value() << 1))
{
    META_FUNCTION_TASK();
}

bool ProgramArgument::operator==(const ProgramArgument& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_hash, m_shader_type, m_name) ==
           std::tie(other.m_hash, other.m_shader_type, other.m_name);
}

ProgramArgument::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    return fmt::format("{} shaders argument '{}'", magic_enum::enum_name(m_shader_type), m_name);
}

ProgramArgumentAccess::ProgramArgumentAccess() noexcept
    : mask(0U)
{
}

ProgramArgumentAccess::ProgramArgumentAccess(uint32_t mask) noexcept
    : mask(mask)
{
}

ProgramArgumentAccess::ProgramArgumentAccess(const std::initializer_list<Type>& types)
    : mask(0U)
{
    META_FUNCTION_TASK();
    for(Type type : types)
    {
        SetType(type, true);
    }
}

void ProgramArgumentAccess::SetType(Type type, bool value)
{
    switch(type)
    {
    case Type::Constant:      is_constant       = value; break;
    case Type::FrameConstant: is_frame_constant = value; break;
    case Type::Mutable:       is_mutable        = value; break;
    default: META_UNEXPECTED_ARG(type);
    }
}

std::vector<ProgramArgumentAccess::Type> ProgramArgumentAccess::GetTypes() const
{
    META_FUNCTION_TASK();
    std::vector<Type> types;
    if (is_constant)
        types.push_back(Type::Constant);
    if (is_frame_constant)
        types.push_back(Type::FrameConstant);
    if (is_mutable)
        types.push_back(Type::Mutable);
    return types;
}

std::vector<std::string> ProgramArgumentAccess::GetTypeNames() const
{
    META_FUNCTION_TASK();
    const std::vector<Type> types = GetTypes();
    std::vector<std::string> type_names;
    for(Type type : types)
    {
        type_names.emplace_back(magic_enum::enum_name(type));
    }
    return type_names;
}

ProgramArgumentAccessor::ProgramArgumentAccessor(ShaderType shader_type, std::string_view argument_name, Type accessor_type, bool addressable) noexcept
    : ProgramArgument(shader_type, argument_name)
    , m_accessor_type(accessor_type)
    , m_addressable(addressable)
{
    META_FUNCTION_TASK();
}

ProgramArgumentAccessor::ProgramArgumentAccessor(const ProgramArgument& argument, Type accessor_type, bool addressable) noexcept
    : ProgramArgument(argument)
    , m_accessor_type(accessor_type)
    , m_addressable(addressable)
{
    META_FUNCTION_TASK();
}

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
    , m_argument_ptr(std::make_unique<IProgram::Argument>(argument))
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics::Rhi
