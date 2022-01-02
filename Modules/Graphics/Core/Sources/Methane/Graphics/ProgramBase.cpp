/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/ProgramBase.cpp
Base implementation of the program interface.

******************************************************************************/

#include "ProgramBase.h"
#include "RenderContextBase.h"
#include "CoreFormatters.hpp"

#include <Methane/Instrumentation.h>
#include <Methane/Platform/Utils.h>

#include <magic_enum.hpp>
#include <algorithm>

namespace Methane::Graphics
{

static const std::hash<std::string_view> g_argument_name_hash;

Program::Argument::Argument(Shader::Type shader_type, std::string_view argument_name) noexcept
    : m_shader_type(shader_type)
    , m_name(argument_name)
    , m_hash(g_argument_name_hash(m_name) ^ (magic_enum::enum_index(shader_type).value() << 1))
{
    META_FUNCTION_TASK();
}

bool Program::Argument::operator==(const Argument& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_hash, m_shader_type, m_name) ==
           std::tie(other.m_hash, other.m_shader_type, other.m_name);
}

Program::Argument::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    return fmt::format("{} shaders argument '{}'", magic_enum::enum_name(m_shader_type), m_name);
}

Program::ArgumentAccessor::ArgumentAccessor(Shader::Type shader_type, std::string_view argument_name, Type accessor_type, bool addressable) noexcept
    : Argument(shader_type, argument_name)
    , m_accessor_type(accessor_type)
    , m_addressable(addressable)
{
    META_FUNCTION_TASK();
}

Program::ArgumentAccessor::ArgumentAccessor(const Argument& argument, Type accessor_type, bool addressable) noexcept
    : Argument(argument)
    , m_accessor_type(accessor_type)
    , m_addressable(addressable)
{
    META_FUNCTION_TASK();
}

size_t Program::ArgumentAccessor::GetAccessorIndex() const noexcept
{
    return magic_enum::enum_index(m_accessor_type).value();
}

Program::ArgumentAccessor::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    return fmt::format("{} ({}{})", Argument::operator std::string(), magic_enum::enum_name(m_accessor_type), (m_addressable ? ", Addressable" : ""));
}

Program::ArgumentAccessors::const_iterator Program::FindArgumentAccessor(const ArgumentAccessors& argument_accessors, const Argument& argument)
{
    META_FUNCTION_TASK();
    if (const auto argument_desc_it = argument_accessors.find(argument);
        argument_desc_it != argument_accessors.end())
        return argument_desc_it;

    const Argument all_shaders_argument(Shader::Type::All, argument.GetName());
    return argument_accessors.find(all_shaders_argument);
}

Program::Argument::NotFoundException::NotFoundException(const Program& program, const Argument& argument)
    : std::invalid_argument(fmt::format("Program '{}' does not have argument '{}' of {} shader.",
                                        program.GetName(), argument.GetName(), magic_enum::enum_name(argument.GetShaderType())))
    , m_program(program)
    , m_argument_ptr(std::make_unique<Program::Argument>(argument))
{
    META_FUNCTION_TASK();
}

ProgramBase::ShadersByType ProgramBase::CreateShadersByType(const Ptrs<Shader>& shaders)
{
    META_FUNCTION_TASK();
    ProgramBase::ShadersByType shaders_by_type;
    for (const Ptr<Shader>& shader_ptr : shaders)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(shader_ptr, "can not use empty shader pointer for program creation");
        shaders_by_type[magic_enum::enum_index(shader_ptr->GetType()).value()] = shader_ptr;
    }
    return shaders_by_type;
}

Shader::Types CreateShaderTypes(const Ptrs<Shader>& shaders)
{
    META_FUNCTION_TASK();
    Shader::Types shader_types;
    for (const Ptr<Shader>& shader_ptr : shaders)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(shader_ptr, "can not use empty shader pointer for program creation");
        shader_types.insert(shader_ptr->GetType());
    }
    return shader_types;
}

ProgramBase::ProgramBase(const ContextBase& context, const Settings& settings)
    : m_context(context)
    , m_settings(settings)
    , m_shaders_by_type(CreateShadersByType(settings.shaders))
    , m_shader_types(CreateShaderTypes(settings.shaders))
{
    META_FUNCTION_TASK();
}

void ProgramBase::InitArgumentBindings(const ArgumentAccessors& argument_accessors)
{
    META_FUNCTION_TASK();
    Shader::Types all_shader_types;
    std::map<std::string_view, Shader::Types, std::less<>> shader_types_by_argument_name_map;
    
    m_binding_by_argument.clear();
    for (const Ptr<Shader>& shader_ptr : m_settings.shaders)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(shader_ptr, "empty shader pointer in program is not allowed");
        const Shader::Type shader_type = shader_ptr->GetType();
        all_shader_types.insert(shader_type);
        
        const ShaderBase::ArgumentBindings argument_bindings = static_cast<const ShaderBase&>(*shader_ptr).GetArgumentBindings(argument_accessors);
        for (const Ptr<ProgramBindingsBase::ArgumentBindingBase>& argument_binging_ptr : argument_bindings)
        {
            META_CHECK_ARG_NOT_NULL_DESCR(argument_binging_ptr, "empty resource binding provided by shader");
            const Argument& shader_argument = argument_binging_ptr->GetSettings().argument;
            m_binding_by_argument.try_emplace(shader_argument, argument_binging_ptr);
            shader_types_by_argument_name_map[shader_argument.GetName()].insert(shader_argument.GetShaderType());
        }
    }
    
    // Replace bindings for argument set for all shader types in program to one binding set for argument with Shader::Type::All
    for (const auto& [argument_name, shader_types] : shader_types_by_argument_name_map)
    {
        if (shader_types != all_shader_types)
            continue;

        Ptr<ProgramBindingsBase::ArgumentBindingBase> argument_binding_ptr;
        for(Shader::Type shader_type : all_shader_types)
        {
            const Argument argument{ shader_type, argument_name };
            auto binding_by_argument_it = m_binding_by_argument.find(argument);
            META_CHECK_ARG_DESCR(argument, binding_by_argument_it != m_binding_by_argument.end(), "Resource binding was not initialized for for argument");
            if (!argument_binding_ptr)
            {
                argument_binding_ptr = binding_by_argument_it->second;
            }
            m_binding_by_argument.erase(binding_by_argument_it);
        }

        META_CHECK_ARG_NOT_NULL_DESCR(argument_binding_ptr, "failed to create resource binding for argument '{}'", argument_name);
        m_binding_by_argument.try_emplace( Argument{ Shader::Type::All, argument_name }, argument_binding_ptr);
    }

    if (m_context.GetType() != Context::Type::Render)
        return;

    // Create frame-constant argument bindings only when program is created in render context
    m_frame_bindings_by_argument.clear();
    const auto& render_context = static_cast<const RenderContextBase&>(m_context);
    const uint32_t frame_buffers_count = render_context.GetSettings().frame_buffers_count;
    META_CHECK_ARG_GREATER_OR_EQUAL(frame_buffers_count, 2);

    for (const auto& [program_argument, argument_binding_ptr] : m_binding_by_argument)
    {
        if (!argument_binding_ptr->GetSettings().argument.IsFrameConstant())
            continue;

        Ptrs<ProgramBindingsBase::ArgumentBindingBase> per_frame_argument_bindings(frame_buffers_count);
        per_frame_argument_bindings[0] = argument_binding_ptr;
        for(uint32_t frame_index = 1; frame_index < frame_buffers_count; ++frame_index)
        {
            per_frame_argument_bindings[frame_index] = ProgramBindingsBase::ArgumentBindingBase::CreateCopy(*argument_binding_ptr);
        }
        m_frame_bindings_by_argument.try_emplace(program_argument, std::move(per_frame_argument_bindings));
    }
}

const Ptr<ProgramBindingsBase::ArgumentBindingBase>& ProgramBase::GetFrameArgumentBinding(Data::Index frame_index, const Program::ArgumentAccessor& argument_accessor) const
{
    META_FUNCTION_TASK();
    const auto argument_frame_bindings_it = m_frame_bindings_by_argument.find(argument_accessor);
    META_CHECK_ARG_TRUE_DESCR(argument_frame_bindings_it != m_frame_bindings_by_argument.end(), "can not find frame-constant argument binding in program");
    return argument_frame_bindings_it->second.at(frame_index);
}

Ptr<ProgramBindingsBase::ArgumentBindingBase> ProgramBase::CreateArgumentBindingInstance(const Ptr<ProgramBindingsBase::ArgumentBindingBase>& argument_binding_ptr, Data::Index frame_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(argument_binding_ptr);

    const Program::ArgumentAccessor& argument_accessor = argument_binding_ptr->GetSettings().argument;
    switch(argument_accessor.GetAccessorType())
    {
    case ArgumentAccessor::Type::Mutable:       return ProgramBindingsBase::ArgumentBindingBase::CreateCopy(*argument_binding_ptr);
    case ArgumentAccessor::Type::Constant:      return argument_binding_ptr;
    case ArgumentAccessor::Type::FrameConstant: return GetFrameArgumentBinding(frame_index, argument_accessor);
    default:                                    META_UNEXPECTED_ARG_RETURN(argument_accessor.GetAccessorType(), nullptr);
    }
}

Shader& ProgramBase::GetShaderRef(Shader::Type shader_type) const
{
    META_FUNCTION_TASK();
    const Ptr<Shader>& shader_ptr = GetShader(shader_type);
    META_CHECK_ARG_DESCR(shader_type, shader_ptr, "{} shader was not found in program '{}'", magic_enum::enum_name(shader_type), GetName());
    return *shader_ptr;
}

uint32_t ProgramBase::GetInputBufferIndexByArgumentSemantic(const std::string& argument_semantic) const
{
    META_FUNCTION_TASK();
    for (size_t buffer_index = 0; buffer_index < m_settings.input_buffer_layouts.size(); buffer_index++)
    {
        const InputBufferLayout& input_buffer_layout = m_settings.input_buffer_layouts[buffer_index];
        if (auto argument_it = std::find(input_buffer_layout.argument_semantics.begin(), input_buffer_layout.argument_semantics.end(), argument_semantic);
            argument_it != input_buffer_layout.argument_semantics.end())
            return static_cast<uint32_t>(buffer_index);
    }
    META_INVALID_ARG_DESCR(argument_semantic, "input argument with semantic name was not found for program '{}'", GetName());
#ifndef METHANE_CHECKS_ENABLED
    return 0;
#endif
}

} // namespace Methane::Graphics
