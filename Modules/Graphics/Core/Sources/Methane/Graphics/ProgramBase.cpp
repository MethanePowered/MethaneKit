/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
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

#include <Methane/Instrumentation.h>
#include <Methane/Platform/Utils.h>

#include <cassert>

namespace Methane::Graphics
{

static const std::hash<std::string> g_argument_name_hash;

Program::Argument::Argument(Shader::Type shader_type, std::string argument_name)
    : shader_type(shader_type)
    , name(std::move(argument_name))
    , hash(g_argument_name_hash(name) ^ (static_cast<size_t>(shader_type) << 1))
{
    ITT_FUNCTION_TASK();
}

bool Program::Argument::operator==(const Argument& other) const
{
    ITT_FUNCTION_TASK();
    return std::tie(hash, shader_type, name) ==
           std::tie(other.hash, other.shader_type, other.name);
}

Program::ArgumentDesc::ArgumentDesc(Shader::Type shader_type, std::string argument_name, Modifiers::Mask modifiers)
    : Argument(shader_type, argument_name)
    , modifiers(modifiers)
{
    ITT_FUNCTION_TASK();
}

Program::ArgumentDesc::ArgumentDesc(const Argument& argument, Modifiers::Mask modifiers)
    : Argument(argument)
    , modifiers(modifiers)
{
    ITT_FUNCTION_TASK();
}

Program::ArgumentDescriptions::const_iterator Program::FindArgumentDescription(const ArgumentDescriptions& argument_descriptions, const Argument& argument)
{
    ITT_FUNCTION_TASK();

    Program::ArgumentDescriptions::const_iterator argument_desc_it = argument_descriptions.find(argument);
    if (argument_desc_it != argument_descriptions.end())
        return argument_desc_it;

    const Argument all_shaders_argument(Shader::Type::All, argument.name);
    return argument_descriptions.find(all_shaders_argument);
}

ProgramBase::ShadersByType ProgramBase::CreateShadersByType(const Ptrs<Shader>& shaders)
{
    ITT_FUNCTION_TASK();

    ProgramBase::ShadersByType shaders_by_type;
    for (const Ptr<Shader>& sp_shader : shaders)
    {
        if (!sp_shader)
        {
            throw std::runtime_error("Can not use empty shader pointer for program creation.");
        }
        
        shaders_by_type[static_cast<size_t>(sp_shader->GetType())] = sp_shader;
    }
    return shaders_by_type;
}

Shader::Types CreateShaderTypes(const Ptrs<Shader>& shaders)
{
    ITT_FUNCTION_TASK();

    Shader::Types shader_types;
    for (const Ptr<Shader>& sp_shader : shaders)
    {
        if (!sp_shader)
        {
            throw std::runtime_error("Can not use empty shader pointer for program creation.");
        }
        
        shader_types.insert(sp_shader->GetType());
    }
    return shader_types;
}

ProgramBase::ProgramBase(ContextBase& context, const Settings& settings)
    : m_context(context)
    , m_settings(settings)
    , m_shaders_by_type(CreateShadersByType(settings.shaders))
    , m_shader_types(CreateShaderTypes(settings.shaders))
{
    ITT_FUNCTION_TASK();
}

ProgramBase::~ProgramBase()
{
    ITT_FUNCTION_TASK();

    std::lock_guard<std::mutex> lock_guard(m_constant_descriptor_ranges_reservation_mutex);
    for (auto& heap_type_and_desc_range : m_constant_descriptor_range_by_heap_type)
    {
        DescriptorHeapReservation& heap_reservation = heap_type_and_desc_range.second;
        if (heap_reservation.range.IsEmpty())
            continue;

        heap_reservation.heap.get().ReleaseRange(heap_reservation.range);
    }
}

void ProgramBase::InitArgumentBindings(const ArgumentDescriptions& argument_descriptions)
{
    ITT_FUNCTION_TASK();

    Shader::Types all_shader_types;
    std::map<std::string, Shader::Types> shader_types_by_argument_name_map;
    
    m_binding_by_argument.clear();
    for (const Ptr<Shader>& sp_shader : m_settings.shaders)
    {
        if (!sp_shader)
        {
            throw std::runtime_error("Empty shader pointer in program is not allowed.");
        }
        
        const Shader::Type shader_type = sp_shader->GetType();
        all_shader_types.insert(shader_type);
        
        const ShaderBase::ArgumentBindings argument_bindings = static_cast<const ShaderBase&>(*sp_shader).GetArgumentBindings(argument_descriptions);
        for (const Ptr<ProgramBindingsBase::ArgumentBindingBase>& sp_argument_binging : argument_bindings)
        {
            if (!sp_argument_binging)
            {
                throw std::runtime_error("Empty resource binding provided by shader.");
            }

            const Argument& shader_argument = sp_argument_binging->GetSettings().argument;
            m_binding_by_argument.emplace(shader_argument, sp_argument_binging);
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
        Ptr<ProgramBindings::ArgumentBinding> sp_argument_binding;
        for(Shader::Type shader_type : all_shader_types)
        {
            const Argument argument = { shader_type, argument_name };
            auto binding_by_argument_it = m_binding_by_argument.find(argument);
            if (binding_by_argument_it == m_binding_by_argument.end())
            {
                throw std::runtime_error("Resource binding was not provided for " + Shader::GetTypeName(shader_type) + " shader argument \"" + argument_name + "\"");
            }
            if (!sp_argument_binding)
            {
                sp_argument_binding = binding_by_argument_it->second;
            }
            m_binding_by_argument.erase(binding_by_argument_it);
        }

        if (!sp_argument_binding)
        {
            throw std::runtime_error("Failed to create resource binding for argument \"" + argument_name + "\".");
        }
        m_binding_by_argument.emplace( Argument{ Shader::Type::All, argument_name }, sp_argument_binding);
    }
}

const DescriptorHeap::Range& ProgramBase::ReserveConstantDescriptorRange(DescriptorHeap& heap, uint32_t range_length)
{
    ITT_FUNCTION_TASK();

    std::lock_guard<std::mutex> lock_guard(m_constant_descriptor_ranges_reservation_mutex);

    const DescriptorHeap::Type heap_type = heap.GetSettings().type;
    auto constant_descriptor_range_by_heap_type_it = m_constant_descriptor_range_by_heap_type.find(heap_type);
    if (constant_descriptor_range_by_heap_type_it != m_constant_descriptor_range_by_heap_type.end())
    {
        const DescriptorHeapReservation& heap_reservation = constant_descriptor_range_by_heap_type_it->second;
        if (std::addressof(heap_reservation.heap.get()) != std::addressof(heap))
        {
            throw std::runtime_error("Constant descriptor range was previously reserved for the program on a different descriptor heap of the same type.");
        }
        if (heap_reservation.range.GetLength() != range_length)
        {
            throw std::runtime_error("Constant descriptor range previously reserved for the program differs in length from requested reservation.");
        }
        return heap_reservation.range;
    }

    Ptr<DescriptorHeap::Range> sp_desc_range = heap.ReserveRange(range_length);
    if (!sp_desc_range)
    {
        throw std::runtime_error("Descriptor heap does not have enough space to reserve constant descriptor range of a program.");
    }
    return m_constant_descriptor_range_by_heap_type.emplace(heap_type, DescriptorHeapReservation{ heap, *sp_desc_range }).first->second.range;
}

Shader& ProgramBase::GetShaderRef(Shader::Type shader_type)
{
    ITT_FUNCTION_TASK();

    const Ptr<Shader>& sp_shader = GetShader(shader_type);
    if (!sp_shader)
    {
        throw std::runtime_error(Shader::GetTypeName(shader_type) + "shader was not found in program \"" + GetName() + "\".");
    }
    return *sp_shader;
}

uint32_t ProgramBase::GetInputBufferIndexByArgumentSemantic(const std::string& argument_semantic) const
{
    ITT_FUNCTION_TASK();

    for (size_t buffer_index = 0; buffer_index < m_settings.input_buffer_layouts.size(); buffer_index++)
    {
        const InputBufferLayout& input_buffer_layout = m_settings.input_buffer_layouts[buffer_index];
        auto argument_it = std::find(input_buffer_layout.argument_semantics.begin(), input_buffer_layout.argument_semantics.end(), argument_semantic);
        if (argument_it != input_buffer_layout.argument_semantics.end())
            return static_cast<uint32_t>(buffer_index);
    }

    throw std::runtime_error("Input argument with semantic \"" + argument_semantic + "\" was not found for program \"" + GetName() + "\"");
}

} // namespace Methane::Graphics
