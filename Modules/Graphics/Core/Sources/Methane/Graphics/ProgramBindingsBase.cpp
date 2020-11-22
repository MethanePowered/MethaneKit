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

FILE: Methane/Graphics/ProgramBindingsBase.cpp
Base implementation of the program bindings interface.

******************************************************************************/

#include "ProgramBindingsBase.h"
#include "ContextBase.h"
#include "CoreFormatters.hpp"

#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Platform/Utils.h>

#include <fmt/ranges.h>
#include <magic_enum.hpp>

namespace Methane::Graphics
{

ProgramBindings::ArgumentBinding::ConstantModificationException::ConstantModificationException()
    : std::logic_error("Can not modify constant program argument binding.")
{
    META_FUNCTION_TASK();
}

ProgramBindings::UnboundArgumentsException::UnboundArgumentsException(const Program& program, const Program::Arguments& unbound_arguments)
    : std::runtime_error(fmt::format("Some arguments of program '{}' are not bound to any resource:\n{}", program.GetName(), unbound_arguments))
    , m_program(program)
    , m_unbound_arguments(unbound_arguments)
{
    META_FUNCTION_TASK();
}

ProgramBindingsBase::ArgumentBindingBase::ArgumentBindingBase(const ContextBase& context, const Settings& settings)
    : m_context(context)
    , m_settings(settings)
{
    META_FUNCTION_TASK();
}

void ProgramBindingsBase::ArgumentBindingBase::SetResourceLocations(const Resource::Locations& resource_locations)
{
    META_FUNCTION_TASK();
    if (m_resource_locations == resource_locations)
        return;

    if (m_settings.argument.IsConstant() && !m_resource_locations.empty())
        throw ConstantModificationException();

    META_CHECK_ARG_NOT_EMPTY_DESCR(resource_locations, "can not set empty resources for resource binding");

    const bool        is_addressable_binding = m_settings.argument.IsAddressable();
    const Resource::Type bound_resource_type = m_settings.resource_type;
    META_UNUSED(is_addressable_binding);
    META_UNUSED(bound_resource_type);

    for (const Resource::Location& resource_location : resource_locations)
    {
        META_CHECK_ARG_NAME_DESCR("resource_location", resource_location.GetResource().GetResourceType() == bound_resource_type,
                                  "incompatible resource type '{}' is bound to argument '{}' of type '{}'",
                                  magic_enum::enum_name(resource_location.GetResource().GetResourceType()),
                                  m_settings.argument.name, magic_enum::enum_name(bound_resource_type));

        const Resource::Usage resource_usage_mask = resource_location.GetResource().GetUsage();
        using namespace magic_enum::bitwise_operators;
        META_CHECK_ARG_DESCR(resource_usage_mask, magic_enum::flags::enum_contains(resource_usage_mask & Resource::Usage::Addressable) == is_addressable_binding,
                             "resource addressable usage flag does not match with resource binding state");
        META_CHECK_ARG_NAME_DESCR("resource_location", is_addressable_binding || !resource_location.GetOffset(),
                                  "can not set resource location with non-zero offset to non-addressable resource binding");
    }

    m_resource_locations = resource_locations;
}

DescriptorHeap::Type ProgramBindingsBase::ArgumentBindingBase::GetDescriptorHeapType() const
{
    META_FUNCTION_TASK();
    return (m_settings.resource_type == Resource::Type::Sampler)
        ? DescriptorHeap::Type::Samplers
        : DescriptorHeap::Type::ShaderResources;
}

bool ProgramBindingsBase::ArgumentBindingBase::IsAlreadyApplied(const Program& program,
                                                                const ProgramBindingsBase& applied_program_bindings,
                                                                bool check_binding_value_changes) const
{
    META_FUNCTION_TASK();

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

ProgramBindingsBase::ProgramBindingsBase(const Ptr<Program>& program_ptr, const ResourceLocationsByArgument& resource_locations_by_argument)
    : m_program_ptr(program_ptr)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_ZERO(program_ptr);

    ReserveDescriptorHeapRanges();
    SetResourcesForArguments(resource_locations_by_argument);
    VerifyAllArgumentsAreBoundToResources();
}

ProgramBindingsBase::ProgramBindingsBase(const ProgramBindingsBase& other_program_bindings, const ResourceLocationsByArgument& replace_resource_locations_by_argument)
    : ObjectBase(other_program_bindings)
    , m_program_ptr(other_program_bindings.m_program_ptr)
    , m_descriptor_heap_reservations_by_type(other_program_bindings.m_descriptor_heap_reservations_by_type)
{
    META_FUNCTION_TASK();

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
    META_FUNCTION_TASK();

    // Release mutable descriptor ranges in heaps (constant ranges are released by the program)
    for (auto& descriptor_type_and_heap_reservation : m_descriptor_heap_reservations_by_type)
    {
        if (!descriptor_type_and_heap_reservation)
            continue;

        const DescriptorHeap::Reservation& heap_reservation = *descriptor_type_and_heap_reservation;
        if (!heap_reservation.mutable_range.IsEmpty())
        {
            heap_reservation.heap.get().ReleaseRange(heap_reservation.mutable_range);
        }

        descriptor_type_and_heap_reservation.reset();
    }
}

const Program& ProgramBindingsBase::GetProgram() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_program_ptr);
    return *m_program_ptr;
}

Program& ProgramBindingsBase::GetProgram()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_program_ptr);
    return *m_program_ptr;
}

void ProgramBindingsBase::ReserveDescriptorHeapRanges()
{
    META_FUNCTION_TASK();

    struct DescriptorsCount
    {
        uint32_t constant_count = 0;
        uint32_t mutable_count = 0;
    };

    META_CHECK_ARG_NOT_NULL(m_program_ptr);
    const auto& program = static_cast<const ProgramBase&>(GetProgram());

    // Count the number of constant and mutable descriptors to be allocated in each descriptor heap
    std::map<DescriptorHeap::Type, DescriptorsCount> descriptors_count_by_heap_type;
    for (const auto& binding_by_argument : program.GetArgumentBindings())
    {
        META_CHECK_ARG_NOT_NULL_DESCR(binding_by_argument.second,
                                      "no resource binding is set for an argument '{}' of shader", binding_by_argument.first.name);

        const auto& argument_binding = static_cast<const ArgumentBindingBase&>(*binding_by_argument.second);
        const auto& binding_settings = argument_binding.GetSettings();
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
        META_CHECK_ARG_EQUAL(heap_reservation.heap.get().GetSettings().type, heap_type);
        META_CHECK_ARG_TRUE(heap_reservation.heap.get().GetSettings().shader_visible);

        if (descriptors.constant_count > 0)
        {
            heap_reservation.constant_range = static_cast<ProgramBase&>(*m_program_ptr).ReserveConstantDescriptorRange(heap_reservation.heap.get(), descriptors.constant_count);
        }
        if (descriptors.mutable_count > 0)
        {
            heap_reservation.mutable_range = heap_reservation.heap.get().ReserveRange(descriptors.mutable_count);
            META_CHECK_ARG_NOT_ZERO_DESCR(heap_reservation.mutable_range, "failed to reserve mutable descriptor heap range, descriptor heap is not big enough");
        }
    }
}

void ProgramBindingsBase::SetResourcesForArguments(const ResourceLocationsByArgument& resource_locations_by_argument) const
{
    META_FUNCTION_TASK();

    for (const auto& argument_and_resource_locations : resource_locations_by_argument)
    {
        const Program::Argument argument = argument_and_resource_locations.first;
        const Ptr<ArgumentBinding>& binding_ptr = Get(argument);
        if (!binding_ptr)
        {
#ifdef PROGRAM_IGNORE_MISSING_ARGUMENTS
            continue;
#else
            throw Program::Argument::NotFoundException(*m_program_ptr, argument);
#endif
        }
        binding_ptr->SetResourceLocations(argument_and_resource_locations.second);
    }
}

const Ptr<ProgramBindings::ArgumentBinding>& ProgramBindingsBase::Get(const Program::Argument& shader_argument) const
{
    META_FUNCTION_TASK();

    static const Ptr<ArgumentBinding> s_empty_argument_binding_ptr;
    auto binding_by_argument_it  = m_binding_by_argument.find(shader_argument);
    return binding_by_argument_it != m_binding_by_argument.end()
         ? binding_by_argument_it->second : s_empty_argument_binding_ptr;
}

Program::Arguments ProgramBindingsBase::GetUnboundArguments() const
{
    META_FUNCTION_TASK();
    Program::Arguments unbound_arguments;
    for (const auto& binding_by_argument : m_binding_by_argument)
    {
        if (binding_by_argument.second->GetResourceLocations().empty())
        {
            unbound_arguments.insert(binding_by_argument.first);
        }
    }
    return unbound_arguments;
}

void ProgramBindingsBase::VerifyAllArgumentsAreBoundToResources() const
{
    META_FUNCTION_TASK();
    // Verify that resources are set for all program arguments
#ifndef PROGRAM_IGNORE_MISSING_ARGUMENTS
    Program::Arguments unbound_arguments = GetUnboundArguments();
    if (!unbound_arguments.empty())
    {
        throw UnboundArgumentsException(*m_program_ptr, unbound_arguments);
    }
#endif
}

const std::optional<DescriptorHeap::Reservation>& ProgramBindingsBase::GetDescriptorHeapReservationByType(DescriptorHeap::Type heap_type) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_EQUAL(heap_type, DescriptorHeap::Type::Undefined);
    return m_descriptor_heap_reservations_by_type[static_cast<uint32_t>(heap_type)];
}

} // namespace Methane::Graphics
