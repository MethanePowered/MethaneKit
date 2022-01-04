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
#include "ProgramBase.h"
#include "CoreFormatters.hpp"

#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Platform/Utils.h>

#include <magic_enum.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <array>

template<>
struct fmt::formatter<Methane::Graphics::Program::ArgumentAccessor>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Graphics::Program::ArgumentAccessor& rl, FormatContext& ctx) { return format_to(ctx.out(), "{}", static_cast<std::string>(rl)); }
    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

template<>
struct fmt::formatter<Methane::Graphics::Resource::Location>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Graphics::Resource::Location& rl, FormatContext& ctx) { return format_to(ctx.out(), "{}", static_cast<std::string>(rl)); }
    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

namespace Methane::Graphics
{

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

void ProgramBindingsBase::ArgumentBindingBase::MergeSettings(const ArgumentBindingBase& other)
{
    META_FUNCTION_TASK();
    const Settings& settings = other.GetSettings();
    META_CHECK_ARG_EQUAL(settings.argument, m_settings.argument);
    META_CHECK_ARG_EQUAL(settings.resource_type, m_settings.resource_type);
    META_CHECK_ARG_EQUAL(settings.resource_count, m_settings.resource_count);
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

    Data::Emitter<ProgramBindings::IArgumentBindingCallback>::Emit(&ProgramBindings::IArgumentBindingCallback::OnProgramArgumentBindingResourceLocationsChanged, std::cref(*this), std::cref(m_resource_locations), std::cref(resource_locations));

    m_resource_locations = resource_locations;
}

ProgramBindingsBase::ArgumentBindingBase::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("{} is bound to {}", m_settings.argument, fmt::join(m_resource_locations, ", "));
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
    if (const ProgramBindings::ArgumentBinding& previous_argument_argument_binding = applied_program_bindings.Get(m_settings.argument);
        previous_argument_argument_binding.GetResourceLocations() == m_resource_locations)
        return true;

    return false;
}

ProgramBindingsBase::ProgramBindingsBase(const Ptr<Program>& program_ptr, const ResourceLocationsByArgument& resource_locations_by_argument, Data::Index frame_index)
    : ProgramBindingsBase(program_ptr, frame_index)
{
    META_FUNCTION_TASK();
    SetResourcesForArguments(resource_locations_by_argument);
    VerifyAllArgumentsAreBoundToResources();
}

ProgramBindingsBase::ProgramBindingsBase(const ProgramBindingsBase& other_program_bindings, const ResourceLocationsByArgument& replace_resource_locations_by_argument, const Opt<Data::Index>& frame_index)
    : ProgramBindingsBase(other_program_bindings, frame_index)
{
    META_FUNCTION_TASK();
    SetResourcesForArguments(ReplaceResourceLocations(other_program_bindings.GetArgumentBindings(), replace_resource_locations_by_argument));
    VerifyAllArgumentsAreBoundToResources();
}

ProgramBindingsBase::ProgramBindingsBase(const Ptr<Program>& program_ptr, Data::Index frame_index)
    : m_program_ptr(program_ptr)
    , m_frame_index(frame_index)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_ZERO(program_ptr);
    InitializeArgumentBindings();
}

ProgramBindingsBase::ProgramBindingsBase(const ProgramBindingsBase& other_program_bindings, const Opt<Data::Index>& frame_index)
    : ObjectBase(other_program_bindings)
    , Data::Receiver<ProgramBindings::IArgumentBindingCallback>()
    , m_program_ptr(other_program_bindings.m_program_ptr)
    , m_frame_index(frame_index.value_or(other_program_bindings.m_frame_index))
{
    META_FUNCTION_TASK();
    InitializeArgumentBindings();
}

Program& ProgramBindingsBase::GetProgram() const
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

void ProgramBindingsBase::OnProgramArgumentBindingResourceLocationsChanged(const ArgumentBinding&, const Resource::Locations&, const Resource::Locations&)
{
    META_FUNCTION_TASK();
    // Implementation is API specific, not handled by default
}

void ProgramBindingsBase::InitializeArgumentBindings()
{
    META_FUNCTION_TASK();
    const auto& program = static_cast<const ProgramBase&>(GetProgram());
    for (const auto& [program_argument, argument_binding_ptr] : program.GetArgumentBindings())
    {
        META_CHECK_ARG_NOT_NULL_DESCR(argument_binding_ptr, "no resource binding is set for program argument '{}'", program_argument.GetName());
        m_arguments.insert(program_argument);
        if (m_binding_by_argument.count(program_argument))
            continue;

        Ptr<ProgramBindingsBase::ArgumentBindingBase> argument_binding_instance_ptr = program.CreateArgumentBindingInstance(argument_binding_ptr, m_frame_index);
        if (argument_binding_ptr->GetSettings().argument.GetAccessorType() == Program::ArgumentAccessor::Type::Mutable)
            argument_binding_instance_ptr->Connect(*this);

        m_binding_by_argument.try_emplace(program_argument, std::move(argument_binding_instance_ptr));
    }
}

ProgramBindings::ResourceLocationsByArgument ProgramBindingsBase::ReplaceResourceLocations(const ArgumentBindings& argument_bindings,
                                                                                           const ResourceLocationsByArgument& replace_resource_locations)
{
    META_FUNCTION_TASK();
    ResourceLocationsByArgument resource_locations_by_argument = replace_resource_locations;
    for (const auto& [program_argument, argument_binding_ptr] : argument_bindings)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(argument_binding_ptr, "no resource binding is set for program argument '{}'", program_argument.GetName());

        // NOTE:
        // constant resource bindings are reusing single binding-object for the whole program,
        // so there's no need in setting its value, since it was already set by the original resource binding
        if (argument_binding_ptr->GetSettings().argument.IsConstant() ||
            resource_locations_by_argument.count(program_argument))
            continue;

        resource_locations_by_argument.try_emplace(program_argument, argument_binding_ptr->GetResourceLocations());
    }
    return resource_locations_by_argument;
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

ProgramBindingsBase::operator std::string() const
{
    META_FUNCTION_TASK();
    bool is_first = true;
    std::stringstream ss;

    for (const auto& [program_argument, argument_binding_ptr] : m_binding_by_argument)
    {
        META_CHECK_ARG_NOT_NULL(argument_binding_ptr);
        if (!is_first)
            ss << ";\n";
        ss << "  - " << static_cast<std::string>(*argument_binding_ptr);
        is_first = false;
    }
    ss << ".";

    return ss.str();
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
    if (Program::Arguments unbound_arguments = GetUnboundArguments();
        !unbound_arguments.empty())
    {
        throw UnboundArgumentsException(*m_program_ptr, unbound_arguments);
    }
}

} // namespace Methane::Graphics
