/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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
#include "ResourceManager.h"
#include "Instrumentation.h"

#include <Methane/Platform/Utils.h>

#include <cassert>
#include <sstream>

using namespace Methane;
using namespace Methane::Graphics;

bool Program::Argument::operator<(const Argument& other) const
{
    ITT_FUNCTION_TASK();
    return shader_type == other.shader_type
         ? argument_name < other.argument_name
         : shader_type < other.shader_type;
}

ProgramBase::ResourceBindingsBase::ResourceBindingsBase(const Program::Ptr& sp_program, const ResourceByArgument& resource_by_argument)
    : m_sp_program(sp_program)
{
    ITT_FUNCTION_TASK();

    if (!m_sp_program)
    {
        throw std::runtime_error("Can not create resource bindings for an empty program.");
    }

    ReserveDescriptorHeapRanges();
    SetResourcesForArguments(resource_by_argument);
    VerifyAllArgumentsAreBoundToResources();
}

ProgramBase::ResourceBindingsBase::ResourceBindingsBase(const ResourceBindingsBase& other_resource_bingings, const ResourceByArgument& replace_resource_by_argument)
    : m_sp_program(other_resource_bingings.m_sp_program)
    , m_descriptor_heap_reservations_by_type(other_resource_bingings.m_descriptor_heap_reservations_by_type)
{
    ITT_FUNCTION_TASK();

    // Form map of original resource bindings
    ResourceByArgument resource_by_argument;
    for (const auto& argument_and_resource_binding : other_resource_bingings.m_resource_binding_by_argument)
    {
        resource_by_argument.emplace(argument_and_resource_binding.first, argument_and_resource_binding.second->GetResource());
    }

    // Substitute resources in original bindings list
    for (const auto& argument_and_resource : replace_resource_by_argument)
    {
        resource_by_argument[argument_and_resource.first] = argument_and_resource.second;
    }

    ReserveDescriptorHeapRanges();
    SetResourcesForArguments(resource_by_argument);
    VerifyAllArgumentsAreBoundToResources();
}

ProgramBase::ResourceBindingsBase::~ResourceBindingsBase()
{
    ITT_FUNCTION_TASK();

    // Release mutable descriptor ranges in heaps (constant ranges are released by the program)
    for (const auto& descriptor_type_and_heap_reservation : m_descriptor_heap_reservations_by_type)
    {
        const DescriptorHeap::Reservation& heap_reservation = descriptor_type_and_heap_reservation.second;
        if (heap_reservation.mutable_range.IsEmpty())
            continue;

        heap_reservation.heap.get().ReleaseRange(heap_reservation.mutable_range);
    }
}

void ProgramBase::ResourceBindingsBase::ReserveDescriptorHeapRanges()
{
    ITT_FUNCTION_TASK();

    struct DescriptorsCount
    {
        uint32_t constant_count = 0;
        uint32_t mutable_count = 0;
    };

    assert(!!m_sp_program);
    ProgramBase& program = static_cast<ProgramBase&>(*m_sp_program);

    // Count the number of constant and mutable discriptots to be allocated in each desriptor heap
    std::map<DescriptorHeap::Type, DescriptorsCount> descriptors_count_by_heap_type;
    for (const auto& resource_binding_by_argument : program.m_resource_binding_by_argument)
    {
        if (!resource_binding_by_argument.second)
        {
            throw std::runtime_error("No resource binding is set for an argument \"" + resource_binding_by_argument.first.argument_name + "\" of shader.");
        }

        const Shader::ResourceBinding& resource_binding = *resource_binding_by_argument.second;
        m_arguments.insert(resource_binding_by_argument.first);

        auto resource_binding_by_argument_it = m_resource_binding_by_argument.find(resource_binding_by_argument.first);
        if (resource_binding_by_argument_it == m_resource_binding_by_argument.end())
        {
            m_resource_binding_by_argument.emplace(resource_binding_by_argument.first, Shader::ResourceBinding::CreateCopy(resource_binding));
        }
        else
        {
            resource_binding_by_argument_it->second = Shader::ResourceBinding::CreateCopy(*resource_binding_by_argument_it->second);
        }

        const DescriptorHeap::Type heap_type = static_cast<const ShaderBase::ResourceBindingBase&>(resource_binding).GetDescriptorHeapType();
        DescriptorsCount& descriptors = descriptors_count_by_heap_type[heap_type];
        if (resource_binding.IsConstant())
        {
            descriptors.constant_count += resource_binding.GetResourceCount();
        }
        else
        {
            descriptors.mutable_count += resource_binding.GetResourceCount();
        }
    }

    // Reserve descriptor ranges in heaps for resource bindings state
    ResourceManager& resource_manager = program.GetContext().GetResourceManager();
    for (const auto& descriptor_heap_type_and_count : descriptors_count_by_heap_type)
    {
        const DescriptorHeap::Type heap_type = descriptor_heap_type_and_count.first;
        const DescriptorsCount&  descriptors = descriptor_heap_type_and_count.second;

        auto shader_descriptor_heap_by_type_it = m_descriptor_heap_reservations_by_type.find(heap_type);
        if (shader_descriptor_heap_by_type_it == m_descriptor_heap_reservations_by_type.end())
        {
            bool success = false;
            std::tie(shader_descriptor_heap_by_type_it, success) = m_descriptor_heap_reservations_by_type.emplace(heap_type, DescriptorHeap::Reservation {
                resource_manager.GetDefaultShaderVisibleDescriptorHeap(heap_type),
                DescriptorHeap::Range(0, 0),
                DescriptorHeap::Range(0, 0)
            });
        }

        DescriptorHeap::Reservation& heap_reservation = shader_descriptor_heap_by_type_it->second;
        if (descriptors.constant_count > 0)
        {
            heap_reservation.constant_range = static_cast<ProgramBase&>(*m_sp_program).ReserveConstantDescriptorRange(heap_reservation.heap.get(), descriptors.constant_count);
        }
        if (descriptors.mutable_count > 0)
        {
            DescriptorHeap::RangePtr sp_mutable_heap_range = heap_reservation.heap.get().ReserveRange(descriptors.mutable_count);
            if (!sp_mutable_heap_range)
            {
                throw std::runtime_error("Failed to reserve mutable descriptor heap range. Descriptor heap is not big enough.");
            }
            heap_reservation.mutable_range = *sp_mutable_heap_range;
        }
    }
}

void ProgramBase::ResourceBindingsBase::SetResourcesForArguments(const ResourceByArgument& resource_by_argument)
{
    ITT_FUNCTION_TASK();

    for (const auto& argument_and_resource : resource_by_argument)
    {
        const Shader::ResourceBinding::Ptr& sp_binding = Get(argument_and_resource.first);
        if (!sp_binding)
        {
#ifndef PROGRAM_IGNORE_MISSING_ARGUMENTS
            throw std::runtime_error("Program \"" + m_sp_program->GetName() +
                "\" does not have argument \"" + argument_and_resource.first.argument_name +
                "\" of " + Shader::GetTypeName(argument_and_resource.first.shader_type) + " shader.");
#else
            continue;
#endif
        }
        sp_binding->SetResource(argument_and_resource.second);
    }
}

const Shader::ResourceBinding::Ptr& ProgramBase::ResourceBindingsBase::Get(const Argument& shader_argument) const
{
    ITT_FUNCTION_TASK();

    static const Shader::ResourceBinding::Ptr sp_empty_resource_binding;
    auto   resource_binding_by_argument_it  = m_resource_binding_by_argument.find(shader_argument);
    return resource_binding_by_argument_it != m_resource_binding_by_argument.end()
         ? resource_binding_by_argument_it->second : sp_empty_resource_binding;
}

bool ProgramBase::ResourceBindingsBase::AllArgumentsAreBoundToResources(std::string& missing_args) const
{
    ITT_FUNCTION_TASK();

    std::stringstream log_ss;
    bool all_arguments_are_bound_to_resources = true;
    for (const auto& resource_binding_by_argument : m_resource_binding_by_argument)
    {
        if (!resource_binding_by_argument.second->GetResource())
        {
            log_ss << std::endl 
                   << "   - Program \"" << m_sp_program->GetName()
                   << "\" argument \"" << resource_binding_by_argument.first.argument_name
                   << "\" of " << Shader::GetTypeName(resource_binding_by_argument.first.shader_type)
                   << " shader is not bound to any resource." ;
            all_arguments_are_bound_to_resources = false;
        }
    }

    if (!all_arguments_are_bound_to_resources)
    {
        missing_args = log_ss.str();
        Platform::PrintToDebugOutput(missing_args);
    }
    return all_arguments_are_bound_to_resources;
}

void ProgramBase::ResourceBindingsBase::VerifyAllArgumentsAreBoundToResources()
{
    ITT_FUNCTION_TASK();
    // Verify that resources are set for all program arguments
#ifndef PROGRAM_IGNORE_MISSING_ARGUMENTS
    std::string missing_args;
    if (!AllArgumentsAreBoundToResources(missing_args))
    {
        throw std::runtime_error("Some arguments of program \"" + m_sp_program->GetName() +
                                 "\" are not bound to any resource:\n" + missing_args);
    }
#endif
}

ProgramBase::ShadersByType ProgramBase::CreateShadersByType(const Shaders& shaders)
{
    ITT_FUNCTION_TASK();

    ProgramBase::ShadersByType shaders_by_type;
    for (const Shader::Ptr& sp_shader : shaders)
    {
        if (!sp_shader)
        {
            throw std::runtime_error("Can not use empty shader pointer for program creation.");
        }
        
        shaders_by_type[static_cast<size_t>(sp_shader->GetType())] = sp_shader;
    }
    return shaders_by_type;
}

Shader::Types CreateShaderTypes(const Shaders& shaders)
{
    ITT_FUNCTION_TASK();

    Shader::Types shader_types;
    for (const Shader::Ptr& sp_shader : shaders)
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
    m_context.AddCallback(*this);
}

ProgramBase::~ProgramBase()
{
    ITT_FUNCTION_TASK();
    m_context.RemoveCallback(*this);

    for (auto& heap_type_and_desc_range : m_constant_descriptor_range_by_heap_type)
    {
        DescriptorHeapReservation& heap_reservation = heap_type_and_desc_range.second;
        if (heap_reservation.range.IsEmpty())
            continue;

        heap_reservation.heap.get().ReleaseRange(heap_reservation.range);
    }
}

void ProgramBase::InitResourceBindings(const std::set<std::string>& constant_argument_names)
{
    ITT_FUNCTION_TASK();

    Shader::Types all_shader_types;
    std::map<std::string, Shader::Types> shader_types_by_argument_name_map;
    
    m_resource_binding_by_argument.clear();
    for (const Shader::Ptr& sp_shader : m_settings.shaders)
    {
        if (!sp_shader)
        {
            throw std::runtime_error("Empty shader ponter in program is not allowed.");
        }
        
        const Shader::Type shader_type = sp_shader->GetType();
        all_shader_types.insert(shader_type);
        
        const Shader::ResourceBindings shader_resource_bindings = static_cast<const ShaderBase&>(*sp_shader).GetResourceBindings(constant_argument_names);
        for (const Shader::ResourceBinding::Ptr& sp_resource_binging : shader_resource_bindings)
        {
            if (!sp_resource_binging)
            {
                throw std::runtime_error("Empty resource binding provided by shader.");
            }

            const Argument shader_argument = { shader_type, sp_resource_binging->GetArgumentName() };
            m_resource_binding_by_argument.emplace(shader_argument, sp_resource_binging);
            shader_types_by_argument_name_map[shader_argument.argument_name].insert(shader_argument.shader_type);
        }
    }
    
    // Replace bindings for argument set for all shader types in program to one binding set for argument with Shader::Type::All
    for (const auto& shader_types_by_argument_name : shader_types_by_argument_name_map)
    {
        const Shader::Types& shader_types = shader_types_by_argument_name.second;
        if (shader_types != all_shader_types)
            continue;

        const std::string& argument_name = shader_types_by_argument_name.first;
        Shader::ResourceBinding::Ptr sp_resource_binding;
        for(Shader::Type shader_type : all_shader_types)
        {
            const Argument argument = { shader_type, argument_name };
            auto resource_binding_by_argument_it = m_resource_binding_by_argument.find(argument);
            if (resource_binding_by_argument_it == m_resource_binding_by_argument.end())
            {
                throw std::runtime_error("Resource binding was not provided for " + Shader::GetTypeName(shader_type) + " shader argument \"" + argument_name + "\"");
            }
            if (!sp_resource_binding)
            {
                sp_resource_binding = resource_binding_by_argument_it->second;
            }
            m_resource_binding_by_argument.erase(resource_binding_by_argument_it);
        }

        if (!sp_resource_binding)
        {
            throw std::runtime_error("Failed to create resource binding for argument \"" + argument_name + "\".");
        }
        m_resource_binding_by_argument.emplace( Argument{ Shader::Type::All, argument_name }, sp_resource_binding);
    }
}

const DescriptorHeap::Range& ProgramBase::ReserveConstantDescriptorRange(DescriptorHeap& heap, uint32_t range_length)
{
    ITT_FUNCTION_TASK();

    const DescriptorHeap::Type heap_type = heap.GetSettings().type;
    auto constant_descriptor_range_by_heap_type_it = m_constant_descriptor_range_by_heap_type.find(heap_type);
    if (constant_descriptor_range_by_heap_type_it != m_constant_descriptor_range_by_heap_type.end())
    {
        const DescriptorHeapReservation& heap_reservation = constant_descriptor_range_by_heap_type_it->second;
        if (std::addressof(heap_reservation.heap.get()) != std::addressof(heap))
        {
            throw std::runtime_error("Constant descriptor range was previously reserver for the program on a different descriptor heap of the same type.");
        }
        if (heap_reservation.range.GetLength() != range_length)
        {
            throw std::runtime_error("Constant descriptor range previously reserved for the program differs in length from requested reservation.");
        }
        return heap_reservation.range;
    }

    DescriptorHeap::RangePtr sp_desc_range = heap.ReserveRange(range_length);
    if (!sp_desc_range)
    {
        throw std::runtime_error("Descriptor heap does not have enough space to reserve constant descriptor range of a program.");
    }
    return m_constant_descriptor_range_by_heap_type.emplace(heap_type, DescriptorHeapReservation{ heap, *sp_desc_range }).first->second.range;
}

Shader& ProgramBase::GetShaderRef(Shader::Type shader_type)
{
    ITT_FUNCTION_TASK();

    const Shader::Ptr& sp_shader = GetShader(shader_type);
    if (!sp_shader)
    {
        throw std::runtime_error(Shader::GetTypeName(shader_type) + "shader was not found in program \"" + m_name + "\".");
    }
    return *sp_shader;
}

uint32_t ProgramBase::GetInputBufferIndexByArgumentName(const std::string& argument_name) const
{
    ITT_FUNCTION_TASK();

    for (size_t buffer_index = 0; buffer_index < m_settings.input_buffer_layouts.size(); buffer_index++)
    {
        const InputBufferLayout& input_buffer_layout = m_settings.input_buffer_layouts[buffer_index];
        auto argument_it = std::find_if(input_buffer_layout.arguments.begin(), input_buffer_layout.arguments.end(),
                                        [&argument_name](const InputBufferLayout::Argument& argument) -> bool
                                        {
                                            return argument.name == argument_name;
                                        });
        if (argument_it != input_buffer_layout.arguments.end())
            return static_cast<uint32_t>(buffer_index);
    }
    
    throw std::runtime_error("Input argument \"" + argument_name + "\" was not found for program \"" + m_name + "\"");
}

uint32_t ProgramBase::GetInputBufferIndexByArgumentSemantic(const std::string& argument_semantic) const
{
    ITT_FUNCTION_TASK();

    for (size_t buffer_index = 0; buffer_index < m_settings.input_buffer_layouts.size(); buffer_index++)
    {
        const InputBufferLayout& input_buffer_layout = m_settings.input_buffer_layouts[buffer_index];
        auto argument_it = std::find_if(input_buffer_layout.arguments.begin(), input_buffer_layout.arguments.end(),
                                        [&argument_semantic](const InputBufferLayout::Argument& argument) -> bool
                                        {
                                            return argument.semantic == argument_semantic;
                                        });
        if (argument_it != input_buffer_layout.arguments.end())
            return static_cast<uint32_t>(buffer_index);
    }

    throw std::runtime_error("Input argument with semantic \"" + argument_semantic + "\" was not found for program \"" + m_name + "\"");
}
