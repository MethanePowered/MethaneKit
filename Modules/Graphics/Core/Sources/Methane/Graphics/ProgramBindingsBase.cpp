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

FILE: Methane/Graphics/ProgramBindingsBase.cpp
Base implementation of the program bindings interface.

******************************************************************************/

#include "ProgramBindingsBase.h"
#include "ContextBase.h"

#include <Methane/Instrumentation.h>
#include <Methane/Platform/Utils.h>

#include <cassert>

namespace Methane::Graphics
{

ProgramBindingsBase::ArgumentBindingBase::ArgumentBindingBase(const ContextBase& context, Settings settings)
    : m_context(context)
    , m_settings(std::move(settings))
{
    ITT_FUNCTION_TASK();
}

void ProgramBindingsBase::ArgumentBindingBase::SetResourceLocations(const Resource::Locations& resource_locations)
{
    ITT_FUNCTION_TASK();

    m_resource_locations.clear();
    if (resource_locations.empty())
        throw std::invalid_argument("Can not set empty resources for resource binding.");

    const bool        is_addressable_binding = m_settings.argument.IsAddressable();
    const Resource::Type bound_resource_type = m_settings.resource_type;

    for (const Resource::Location& resource_location : resource_locations)
    {
        if (resource_location.GetResource().GetResourceType() != bound_resource_type)
        {
            throw std::invalid_argument("Incompatible resource type \"" + Resource::GetTypeName(resource_location.GetResource().GetResourceType()) +
                                        "\" is bound to argument \"" + m_settings.argument.name +
                                        "\" of type \"" + Resource::GetTypeName(bound_resource_type) + "\".");
        }

        const Resource::Usage::Mask resource_usage_mask = resource_location.GetResource().GetUsageMask();
        if (static_cast<bool>(resource_usage_mask & Resource::Usage::Addressable) != is_addressable_binding)
            throw std::invalid_argument("Resource addressable usage flag does not match with resource binding state.");

        if (!is_addressable_binding && resource_location.GetOffset() > 0)
            throw std::invalid_argument("Can not set resource location with non-zero offset to non-addressable resource binding.");
    }

    m_resource_locations = resource_locations;
}

DescriptorHeap::Type ProgramBindingsBase::ArgumentBindingBase::GetDescriptorHeapType() const
{
    ITT_FUNCTION_TASK();
    return (m_settings.resource_type == Resource::Type::Sampler)
        ? DescriptorHeap::Type::Samplers
        : DescriptorHeap::Type::ShaderResources;
}

bool ProgramBindingsBase::ArgumentBindingBase::IsAlreadyApplied(const Program& program,
                                                                const ProgramBindingsBase& applied_program_bindings,
                                                                bool check_binding_value_changes) const
{
    ITT_FUNCTION_TASK();

    if (std::addressof(applied_program_bindings.GetProgram()) != std::addressof(program))
        return false;

    // 1) No need in setting constant resource binding
    //    when another binding was previously set in the same command list for the same program
    if (m_settings.argument.IsConstant())
        return true;

    if (!check_binding_value_changes)
        return false;

    const Ptr<ProgramBindings::ArgumentBinding>& previous_argument_argument_binding = applied_program_bindings.Get(m_settings.argument);
    if (!previous_argument_argument_binding)
        return false;

    // 2) No need in setting resource binding to the same location
    //    as a previous resource binding set in the same command list for the same program
    if (previous_argument_argument_binding->GetResourceLocations() == m_resource_locations)
        return true;

    return false;
}

ProgramBindingsBase::ProgramBindingsBase(const Ptr<Program>& sp_program, const ResourceLocationsByArgument& resource_locations_by_argument)
    : m_sp_program(sp_program)
{
    ITT_FUNCTION_TASK();

    if (!m_sp_program)
    {
        throw std::runtime_error("Can not create resource bindings for an empty program.");
    }

    ReserveDescriptorHeapRanges();
    SetResourcesForArguments(resource_locations_by_argument);
    VerifyAllArgumentsAreBoundToResources();
}

ProgramBindingsBase::ProgramBindingsBase(const ProgramBindingsBase& other_program_bindings, const ResourceLocationsByArgument& replace_resource_locations_by_argument)
    : m_sp_program(other_program_bindings.m_sp_program)
    , m_descriptor_heap_reservations_by_type(other_program_bindings.m_descriptor_heap_reservations_by_type)
{
    ITT_FUNCTION_TASK();

    // Form map of volatile resource bindings with replaced resource locations
    ResourceLocationsByArgument resource_locations_by_argument = replace_resource_locations_by_argument;
    for (const auto& argument_and_argument_binding : other_program_bindings.m_binding_by_argument)
    {
        // NOTE: constant resource bindings are reusing single binding-object for the whole program,
        //       so there's no need in setting its value, since it was already set by the original resource binding
        if (argument_and_argument_binding.second->GetSettings().argument.IsConstant() ||
            resource_locations_by_argument.count(argument_and_argument_binding.first))
            continue;

        resource_locations_by_argument.emplace(
            argument_and_argument_binding.first,
            argument_and_argument_binding.second->GetResourceLocations()
        );
    }

    ReserveDescriptorHeapRanges();
    SetResourcesForArguments(resource_locations_by_argument);
    VerifyAllArgumentsAreBoundToResources();
}

ProgramBindingsBase::~ProgramBindingsBase()
{
    ITT_FUNCTION_TASK();

    // Release mutable descriptor ranges in heaps (constant ranges are released by the program)
    for (const auto& descriptor_type_and_heap_reservation : m_descriptor_heap_reservations_by_type)
    {
        if (!descriptor_type_and_heap_reservation)
            continue;

        const DescriptorHeap::Reservation& heap_reservation = *descriptor_type_and_heap_reservation;
        if (heap_reservation.mutable_range.IsEmpty())
            continue;

        heap_reservation.heap.get().ReleaseRange(heap_reservation.mutable_range);
    }
}

const Program& ProgramBindingsBase::GetProgram() const
{
    ITT_FUNCTION_TASK();
    assert(!!m_sp_program);
    return *m_sp_program;
}

Program& ProgramBindingsBase::GetProgram()
{
    ITT_FUNCTION_TASK();
    assert(!!m_sp_program);
    return *m_sp_program;
}

void ProgramBindingsBase::ReserveDescriptorHeapRanges()
{
    ITT_FUNCTION_TASK();

    struct DescriptorsCount
    {
        uint32_t constant_count = 0;
        uint32_t mutable_count = 0;
    };

    assert(!!m_sp_program);
    const ProgramBase& program = static_cast<const ProgramBase&>(GetProgram());

    // Count the number of constant and mutable descriptors to be allocated in each descriptor heap
    std::map<DescriptorHeap::Type, DescriptorsCount> descriptors_count_by_heap_type;
    for (const auto& binding_by_argument : program.GetArgumentBindings())
    {
        if (!binding_by_argument.second)
        {
            throw std::runtime_error("No resource binding is set for an argument \"" + binding_by_argument.first.name + "\" of shader.");
        }

        const ArgumentBindingBase&       argument_binding = static_cast<const ArgumentBindingBase&>(*binding_by_argument.second);
        const ArgumentBinding::Settings& binding_settings = argument_binding.GetSettings();
        m_arguments.insert(binding_by_argument.first);

        auto binding_by_argument_it = m_binding_by_argument.find(binding_by_argument.first);
        if (binding_by_argument_it == m_binding_by_argument.end())
        {
            m_binding_by_argument.emplace(
                binding_by_argument.first,
                binding_settings.argument.IsConstant()
                    ? binding_by_argument.second
                    : ArgumentBindingBase::CreateCopy(argument_binding)
            );
        }
        else if (!binding_settings.argument.IsConstant())
        {
            binding_by_argument_it->second = ArgumentBindingBase::CreateCopy(static_cast<const ArgumentBindingBase&>(*binding_by_argument_it->second));
        }

        // NOTE: addressable resource bindings do not require descriptors to be created, instead they use direct GPU memory offset from resource
        if (binding_settings.argument.IsAddressable())
            continue;

        const DescriptorHeap::Type heap_type = argument_binding.GetDescriptorHeapType();
        DescriptorsCount& descriptors = descriptors_count_by_heap_type[heap_type];
        if (binding_settings.argument.IsConstant())
        {
            descriptors.constant_count += binding_settings.resource_count;
        }
        else
        {
            descriptors.mutable_count += binding_settings.resource_count;
        }
    }

    // Reserve descriptor ranges in heaps for resource bindings state
    const ResourceManager& resource_manager = program.GetContext().GetResourceManager();
    for (const auto& descriptor_heap_type_and_count : descriptors_count_by_heap_type)
    {
        const DescriptorHeap::Type heap_type = descriptor_heap_type_and_count.first;
        const DescriptorsCount&  descriptors = descriptor_heap_type_and_count.second;

        std::optional<DescriptorHeap::Reservation>& descriptor_heap_reservation_opt = m_descriptor_heap_reservations_by_type[static_cast<uint32_t>(heap_type)];
        if (!descriptor_heap_reservation_opt)
        {
            descriptor_heap_reservation_opt.emplace(
                resource_manager.GetDefaultShaderVisibleDescriptorHeap(heap_type),
                DescriptorHeap::Range(0, 0),
                DescriptorHeap::Range(0, 0)
            );
        }

        DescriptorHeap::Reservation& heap_reservation = *descriptor_heap_reservation_opt;
        if (descriptors.constant_count > 0)
        {
            heap_reservation.constant_range = static_cast<ProgramBase&>(*m_sp_program).ReserveConstantDescriptorRange(heap_reservation.heap.get(), descriptors.constant_count);
        }
        if (descriptors.mutable_count > 0)
        {
            Ptr<DescriptorHeap::Range> sp_mutable_heap_range = heap_reservation.heap.get().ReserveRange(descriptors.mutable_count);
            if (!sp_mutable_heap_range)
            {
                throw std::runtime_error("Failed to reserve mutable descriptor heap range. Descriptor heap is not big enough.");
            }
            heap_reservation.mutable_range = *sp_mutable_heap_range;
        }
    }
}

void ProgramBindingsBase::SetResourcesForArguments(const ResourceLocationsByArgument& resource_locations_by_argument)
{
    ITT_FUNCTION_TASK();

    for (const auto& argument_and_resource_locations : resource_locations_by_argument)
    {
        const Program::Argument argument = argument_and_resource_locations.first;
        const Ptr<ArgumentBinding>& sp_binding = Get(argument);
        if (!sp_binding)
        {
#ifndef PROGRAM_IGNORE_MISSING_ARGUMENTS
            const Program::Argument all_shaders_argument(Shader::Type::All, argument.name);
            const bool all_shaders_argument_found = !!Get(all_shaders_argument);
            throw std::runtime_error("Program \"" + m_sp_program->GetName() +
                                     "\" does not have argument \"" + argument.name +
                                     "\" of " + Shader::GetTypeName(argument.shader_type) + " shader." +
                                     (all_shaders_argument_found ? " Instead this argument is used in All shaders." : "") );
#else
            continue;
#endif
        }
        sp_binding->SetResourceLocations(argument_and_resource_locations.second);
    }
}

const Ptr<ProgramBindings::ArgumentBinding>& ProgramBindingsBase::Get(const Program::Argument& shader_argument) const
{
    ITT_FUNCTION_TASK();

    static const Ptr<ArgumentBinding> s_sp_empty_argument_binding;
    auto binding_by_argument_it  = m_binding_by_argument.find(shader_argument);
    return binding_by_argument_it != m_binding_by_argument.end()
         ? binding_by_argument_it->second : s_sp_empty_argument_binding;
}

bool ProgramBindingsBase::AllArgumentsAreBoundToResources(std::string& missing_args) const
{
    ITT_FUNCTION_TASK();

    std::stringstream log_ss;
    bool all_arguments_are_bound_to_resources = true;
    for (const auto& binding_by_argument : m_binding_by_argument)
    {
        const Resource::Locations& resource_locations = binding_by_argument.second->GetResourceLocations();
        if (resource_locations.empty())
        {
            log_ss << std::endl 
                   << "   - Program \"" << m_sp_program->GetName()
                   << "\" argument \"" << binding_by_argument.first.name
                   << "\" of " << Shader::GetTypeName(binding_by_argument.first.shader_type)
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

void ProgramBindingsBase::VerifyAllArgumentsAreBoundToResources()
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

const std::optional<DescriptorHeap::Reservation>& ProgramBindingsBase::GetDescriptorHeapReservationByType(DescriptorHeap::Type heap_type) const
{
    ITT_FUNCTION_TASK();
    assert(heap_type != DescriptorHeap::Type::Undefined);
    return m_descriptor_heap_reservations_by_type[static_cast<uint32_t>(heap_type)];
}

} // namespace Methane::Graphics
