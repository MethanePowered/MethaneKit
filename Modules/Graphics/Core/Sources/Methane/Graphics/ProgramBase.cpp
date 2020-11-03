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
#include "ContextBase.h"
#include "Formatters.hpp"

#include <Methane/Instrumentation.h>
#include <Methane/Platform/Utils.h>

#include <cassert>

namespace Methane::Graphics
{

static const std::hash<std::string> g_argument_name_hash;

Program::Argument::Argument(Shader::Type shader_type, std::string argument_name) noexcept
    : shader_type(shader_type)
    , name(std::move(argument_name))
    , hash(g_argument_name_hash(name) ^ (static_cast<size_t>(shader_type) << 1))
{
    META_FUNCTION_TASK();
}

bool Program::Argument::operator==(const Argument& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(hash, shader_type, name) ==
           std::tie(other.hash, other.shader_type, other.name);
}

Program::Argument::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    return fmt::format("{} {}", Shader::GetTypeName(shader_type), name);
}

Program::ArgumentDesc::ArgumentDesc(Shader::Type shader_type, std::string argument_name, Modifiers::Mask modifiers) noexcept
    : Argument(shader_type, argument_name)
    , modifiers(modifiers)
{
    META_FUNCTION_TASK();
}

Program::ArgumentDesc::ArgumentDesc(const Argument& argument, Modifiers::Mask modifiers) noexcept
    : Argument(argument)
    , modifiers(modifiers)
{
    META_FUNCTION_TASK();
}

Program::ArgumentDescriptions::const_iterator Program::FindArgumentDescription(const ArgumentDescriptions& argument_descriptions, const Argument& argument)
{
    META_FUNCTION_TASK();

    Program::ArgumentDescriptions::const_iterator argument_desc_it = argument_descriptions.find(argument);
    if (argument_desc_it != argument_descriptions.end())
        return argument_desc_it;

    const Argument all_shaders_argument(Shader::Type::All, argument.name);
    return argument_descriptions.find(all_shaders_argument);
}

Program::Argument::NotFoundException::NotFoundException(const Program& program, const Argument& argument)
    : std::invalid_argument(fmt::format("Program '{}' does not have argument '{} of {} shader.",
                                        program.GetName(), argument.name, Shader::GetTypeName(argument.shader_type)))
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
        shaders_by_type[static_cast<size_t>(shader_ptr->GetType())] = shader_ptr;
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

ProgramBase::ProgramBase(ContextBase& context, const Settings& settings)
    : m_context(context)
    , m_settings(settings)
    , m_shaders_by_type(CreateShadersByType(settings.shaders))
    , m_shader_types(CreateShaderTypes(settings.shaders))
{
    META_FUNCTION_TASK();
}

ProgramBase::~ProgramBase()
{
    META_FUNCTION_TASK();

    std::lock_guard<LockableBase(std::mutex)> lock_guard(m_constant_descriptor_ranges_reservation_mutex);
    for (const auto& heap_type_and_desc_range : m_constant_descriptor_range_by_heap_type)
    {
        const DescriptorHeapReservation& heap_reservation = heap_type_and_desc_range.second;
        if (heap_reservation.range.IsEmpty())
            continue;

        heap_reservation.heap.get().ReleaseRange(heap_reservation.range);
    }
}

void ProgramBase::InitArgumentBindings(const ArgumentDescriptions& argument_descriptions)
{
    META_FUNCTION_TASK();

    Shader::Types all_shader_types;
    std::map<std::string, Shader::Types> shader_types_by_argument_name_map;
    
    m_binding_by_argument.clear();
    for (const Ptr<Shader>& shader_ptr : m_settings.shaders)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(shader_ptr, "empty shader pointer in program is not allowed");
        const Shader::Type shader_type = shader_ptr->GetType();
        all_shader_types.insert(shader_type);
        
        const ShaderBase::ArgumentBindings argument_bindings = static_cast<const ShaderBase&>(*shader_ptr).GetArgumentBindings(argument_descriptions);
        for (const Ptr<ProgramBindingsBase::ArgumentBindingBase>& argument_binging_ptr : argument_bindings)
        {
            META_CHECK_ARG_NOT_NULL_DESCR(argument_binging_ptr, "empty resource binding provided by shader");
            const Argument& shader_argument = argument_binging_ptr->GetSettings().argument;
            m_binding_by_argument.emplace(shader_argument, argument_binging_ptr);
            shader_types_by_argument_name_map[shader_argument.name].insert(shader_argument.shader_type);
        }
    }
    
    // Replace bindings for argument set for all shader types in program to one binding set for argument with Shader::Type::All
    for (const auto& shader_types_by_argument_name : shader_types_by_argument_name_map)
    {
        const Shader::Types& shader_types = shader_types_by_argument_name.second;
        if (shader_types != all_shader_types)
            continue;

        const std::string& argument_name = shader_types_by_argument_name.first;
        Ptr<ProgramBindings::ArgumentBinding> argument_binding_ptr;
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

        META_CHECK_ARG_NOT_NULL_DESCR(argument_binding_ptr, fmt::format("failed to create resource binding for argument '{}'", argument_name));
        m_binding_by_argument.emplace( Argument{ Shader::Type::All, argument_name }, argument_binding_ptr);
    }
}

const DescriptorHeap::Range& ProgramBase::ReserveConstantDescriptorRange(DescriptorHeap& heap, uint32_t range_length)
{
    META_FUNCTION_TASK();
    std::lock_guard<LockableBase(std::mutex)> lock_guard(m_constant_descriptor_ranges_reservation_mutex);

    const DescriptorHeap::Type heap_type = heap.GetSettings().type;
    auto constant_descriptor_range_by_heap_type_it = m_constant_descriptor_range_by_heap_type.find(heap_type);
    if (constant_descriptor_range_by_heap_type_it != m_constant_descriptor_range_by_heap_type.end())
    {
        const DescriptorHeapReservation& heap_reservation = constant_descriptor_range_by_heap_type_it->second;
        META_CHECK_ARG_NAME_DESCR("heap", std::addressof(heap) == std::addressof(heap_reservation.heap.get()),
                                  "constant descriptor range was previously reserved for the program on a different descriptor heap of the same type");
        META_CHECK_ARG_DESCR(range_length, range_length == heap_reservation.range.GetLength(),
                             "constant descriptor range previously reserved for the program differs in length from requested reservation");
        return heap_reservation.range;
    }

    DescriptorHeap::Range const_desc_range = heap.ReserveRange(range_length);
    META_CHECK_ARG_NOT_ZERO_DESCR(const_desc_range, "Descriptor heap does not have enough space to reserve constant descriptor range of a program.");
    return m_constant_descriptor_range_by_heap_type.emplace(heap_type, DescriptorHeapReservation{ heap, const_desc_range }).first->second.range;
}

Shader& ProgramBase::GetShaderRef(Shader::Type shader_type)
{
    META_FUNCTION_TASK();
    const Ptr<Shader>& shader_ptr = GetShader(shader_type);
    META_CHECK_ARG_DESCR(shader_type, shader_ptr, fmt::format("{} shader was not found in program '{}'", Shader::GetTypeName(shader_type), GetName()));
    return *shader_ptr;
}

uint32_t ProgramBase::GetInputBufferIndexByArgumentSemantic(const std::string& argument_semantic) const
{
    META_FUNCTION_TASK();
    for (size_t buffer_index = 0; buffer_index < m_settings.input_buffer_layouts.size(); buffer_index++)
    {
        const InputBufferLayout& input_buffer_layout = m_settings.input_buffer_layouts[buffer_index];
        auto argument_it = std::find(input_buffer_layout.argument_semantics.begin(), input_buffer_layout.argument_semantics.end(), argument_semantic);
        if (argument_it != input_buffer_layout.argument_semantics.end())
            return static_cast<uint32_t>(buffer_index);
    }
    META_INVALID_ARG_DESCR(argument_semantic, fmt::format("input argument with semantic name was not found for program '{}'", GetName()));
}

} // namespace Methane::Graphics
