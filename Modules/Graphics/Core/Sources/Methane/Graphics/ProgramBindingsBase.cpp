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
#include "RenderContextBase.h"
#include "CoreFormatters.hpp"

#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Platform/Utils.h>

#include <fmt/ranges.h>
#include <magic_enum.hpp>
#include <array>

namespace Methane::Graphics
{

DescriptorsCountByAccess::DescriptorsCountByAccess()
{
    std::fill(m_count_by_access_type.begin(), m_count_by_access_type.end(), 0U);
}

uint32_t& DescriptorsCountByAccess::operator[](Program::ArgumentAccessor::Type access_type)
{
    return m_count_by_access_type[magic_enum::enum_index(access_type).value()];
}

uint32_t DescriptorsCountByAccess::operator[](Program::ArgumentAccessor::Type access_type) const
{
    return m_count_by_access_type[magic_enum::enum_index(access_type).value()];
}

ProgramBindings::ArgumentBinding::ConstantModificationException::ConstantModificationException(const Program::Argument& argument)
    : std::logic_error(fmt::format("Can not modify constant argument binding '{}' of {} shaders.",
                                   argument.GetName(), magic_enum::enum_name(argument.GetShaderType())))
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
        throw ConstantModificationException(GetSettings().argument);

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
                                  m_settings.argument.GetName(), magic_enum::enum_name(bound_resource_type));

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

    // 2) No need in setting resource binding to the same location
    //    as a previous resource binding set in the same command list for the same program
    ProgramBindings::ArgumentBinding& previous_argument_argument_binding = applied_program_bindings.Get(m_settings.argument);
    if (previous_argument_argument_binding.GetResourceLocations() == m_resource_locations)
        return true;

    return false;
}

ProgramBindingsBase::ProgramBindingsBase(const Ptr<Program>& program_ptr, const ResourceLocationsByArgument& resource_locations_by_argument, Data::Index frame_index)
    : m_program_ptr(program_ptr)
    , m_frame_index(frame_index)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_ZERO(program_ptr);

    ReserveDescriptorHeapRanges();
    SetResourcesForArguments(resource_locations_by_argument);
    VerifyAllArgumentsAreBoundToResources();
}

ProgramBindingsBase::ProgramBindingsBase(const ProgramBindingsBase& other_program_bindings, const ResourceLocationsByArgument& replace_resource_locations_by_argument, const Opt<Data::Index>& frame_index)
    : ObjectBase(other_program_bindings)
    , m_program_ptr(other_program_bindings.m_program_ptr)
    , m_frame_index(frame_index ? *frame_index : other_program_bindings.m_frame_index)
    , m_descriptor_heap_reservations_by_type(other_program_bindings.m_descriptor_heap_reservations_by_type)
{
    META_FUNCTION_TASK();

    // Form map of volatile resource bindings with replaced resource locations
    ResourceLocationsByArgument resource_locations_by_argument = replace_resource_locations_by_argument;
    for (const auto& [program_argument, argument_binding_ptr] : other_program_bindings.m_binding_by_argument)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(argument_binding_ptr, "no resource binding is set for program argument '{}'", program_argument.GetName());

        // NOTE: constant resource bindings are reusing single binding-object for the whole program,
        //       so there's no need in setting its value, since it was already set by the original resource binding
        if (argument_binding_ptr->GetSettings().argument.IsConstant() ||
            resource_locations_by_argument.count(program_argument))
            continue;

        resource_locations_by_argument.try_emplace(
            program_argument,
            argument_binding_ptr->GetResourceLocations()
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
    for (auto& heap_reservation_opt : m_descriptor_heap_reservations_by_type)
    {
        if (!heap_reservation_opt)
            continue;

        if (const DescriptorHeap::Range& mutable_descriptor_range = heap_reservation_opt->ranges[magic_enum::enum_index(Program::ArgumentAccessor::Type::Mutable).value()];
            !mutable_descriptor_range.IsEmpty())
        {
            heap_reservation_opt->heap.get().ReleaseRange(mutable_descriptor_range);
        }

        heap_reservation_opt.reset();
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
    META_CHECK_ARG_NOT_NULL(m_program_ptr);
    const auto& program = static_cast<const ProgramBase&>(GetProgram());
    const uint32_t frames_count = program.GetContext().GetType() == Context::Type::Render
                                ? dynamic_cast<const RenderContextBase&>(program.GetContext()).GetSettings().frame_buffers_count
                                : 1U;

    // Count the number of constant and mutable descriptors to be allocated in each descriptor heap
    std::map<DescriptorHeap::Type, DescriptorsCountByAccess> descriptors_count_by_heap_type;
    for (const auto& [program_argument, argument_binding_ptr] : program.GetArgumentBindings())
    {
        META_CHECK_ARG_NOT_NULL_DESCR(argument_binding_ptr, "no resource binding is set for program argument '{}'", program_argument.GetName());
        m_arguments.insert(program_argument);
        if (!m_binding_by_argument.count(program_argument))
        {
            m_binding_by_argument.try_emplace(program_argument, program.CreateArgumentBindingInstance(argument_binding_ptr, m_frame_index));
        }

        // NOTE: addressable resource bindings do not require descriptors to be created, instead they use direct GPU memory offset from resource
        const auto& binding_settings = argument_binding_ptr->GetSettings();
        if (binding_settings.argument.IsAddressable())
            continue;

        const DescriptorHeap::Type            heap_type = argument_binding_ptr->GetDescriptorHeapType();
        const Program::ArgumentAccessor::Type access_type = binding_settings.argument.GetAccessorType();

        uint32_t resources_count = binding_settings.resource_count;
        if (access_type == Program::ArgumentAccessor::Type::FrameConstant)
        {
            // For Frame Constant bindings we reserve descriptors range for all frames at once
            resources_count *= frames_count;
        }

        descriptors_count_by_heap_type[heap_type][access_type] += resources_count;
    }

    // Reserve descriptor ranges in heaps for resource bindings state
    const ResourceManager& resource_manager = program.GetContext().GetResourceManager();
    ProgramBase& mutable_program = static_cast<ProgramBase&>(*m_program_ptr);
    for (const auto& [heap_type, descriptors_count] : descriptors_count_by_heap_type)
    {
        std::optional<DescriptorHeap::Reservation>& descriptor_heap_reservation_opt = m_descriptor_heap_reservations_by_type[magic_enum::enum_integer(heap_type)];
        if (!descriptor_heap_reservation_opt)
        {
            descriptor_heap_reservation_opt.emplace(resource_manager.GetDefaultShaderVisibleDescriptorHeap(heap_type));
        }

        DescriptorHeap::Reservation& heap_reservation = *descriptor_heap_reservation_opt;
        META_CHECK_ARG_EQUAL(heap_reservation.heap.get().GetSettings().type, heap_type);
        META_CHECK_ARG_TRUE(heap_reservation.heap.get().GetSettings().shader_visible);

        for (Program::ArgumentAccessor::Type access_type : magic_enum::flags::enum_values<Program::ArgumentAccessor::Type>())
        {
            const uint32_t accessor_descr_count = descriptors_count[access_type];
            if (!accessor_descr_count)
                continue;

            DescriptorHeap::Range& heap_range = heap_reservation.ranges[magic_enum::enum_index(access_type).value()];
            heap_range = mutable_program.ReserveDescriptorRange(heap_reservation.heap.get(), access_type, accessor_descr_count);

            if (access_type == Program::ArgumentAccessor::Type::FrameConstant)
            {
                // Since Frame Constant binding range was reserved for all frames at once
                // we need to take only one sub-range related to the frame of current bindings
                const Data::Index frame_range_length = heap_range.GetLength() / frames_count;
                const Data::Index frame_range_start  = heap_range.GetStart() + frame_range_length * m_frame_index;
                heap_range = DescriptorHeap::Range(frame_range_start, frame_range_start + frame_range_length);
            }
        }
    }
}

void ProgramBindingsBase::SetResourcesForArguments(const ResourceLocationsByArgument& resource_locations_by_argument) const
{
    META_FUNCTION_TASK();
    for (const auto& [program_argument, resource_locations] : resource_locations_by_argument)
    {
        Get(program_argument).SetResourceLocations(resource_locations);
    }
}

ProgramBindings::ArgumentBinding& ProgramBindingsBase::Get(const Program::Argument& shader_argument) const
{
    META_FUNCTION_TASK();
    const auto binding_by_argument_it = m_binding_by_argument.find(shader_argument);
    if (binding_by_argument_it == m_binding_by_argument.end())
        throw Program::Argument::NotFoundException(*m_program_ptr, shader_argument);

    return *binding_by_argument_it->second;
}

Program::Arguments ProgramBindingsBase::GetUnboundArguments() const
{
    META_FUNCTION_TASK();
    Program::Arguments unbound_arguments;
    for (const auto& [program_argument, argument_binding_ptr] : m_binding_by_argument)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(argument_binding_ptr, "no resource binding is set for program argument '{}'", program_argument.GetName());

        if (argument_binding_ptr->GetResourceLocations().empty())
        {
            unbound_arguments.insert(program_argument);
        }
    }
    return unbound_arguments;
}

void ProgramBindingsBase::VerifyAllArgumentsAreBoundToResources() const
{
    META_FUNCTION_TASK();
    // Verify that resources are set for all program arguments
#ifndef PROGRAM_IGNORE_MISSING_ARGUMENTS
    if (Program::Arguments unbound_arguments = GetUnboundArguments();
        !unbound_arguments.empty())
    {
        throw UnboundArgumentsException(*m_program_ptr, unbound_arguments);
    }
#endif
}

const std::optional<DescriptorHeap::Reservation>& ProgramBindingsBase::GetDescriptorHeapReservationByType(DescriptorHeap::Type heap_type) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_EQUAL(heap_type, DescriptorHeap::Type::Undefined);
    return m_descriptor_heap_reservations_by_type[magic_enum::enum_integer(heap_type)];
}

} // namespace Methane::Graphics
