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
#include "ResourceBase.h"
#include "CoreFormatters.hpp"

#include <Methane/Graphics/Buffer.h>
#include <Methane/Graphics/Texture.h>
#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Platform/Utils.h>

#include <magic_enum.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <array>

template<>
struct fmt::formatter<Methane::Graphics::IProgram::ArgumentAccessor>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Graphics::IProgram::ArgumentAccessor& rl, FormatContext& ctx) { return format_to(ctx.out(), "{}", static_cast<std::string>(rl)); }
    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

template<>
struct fmt::formatter<Methane::Graphics::Resource::View>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Graphics::Resource::View& rl, FormatContext& ctx) { return format_to(ctx.out(), "{}", static_cast<std::string>(rl)); }
    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

namespace Methane::Graphics
{

static Resource::State GetBoundResourceTargetState(const Resource& resource, Resource::Type resource_type, bool is_constant_binding)
{
    META_FUNCTION_TASK();
    switch (resource_type)
    {
    case Resource::Type::Buffer:
        // FIXME: state transition of DX upload heap resources should be reworked properly and made friendly with Vulkan
        // DX resource in upload heap can not be transitioned to any other state but initial GenericRead state
        if (dynamic_cast<const Buffer&>(resource).GetSettings().storage_mode != Buffer::StorageMode::Private)
            return resource.GetState();
        else if (is_constant_binding)
            return Resource::State::ConstantBuffer;
        break;

    case Resource::Type::Texture:
        if (dynamic_cast<const Texture&>(resource).GetSettings().type == Texture::Type::DepthStencilBuffer)
            return Resource::State::DepthRead;
        break;

    default:
        break;
    }
    return Resource::State::ShaderResource;
}

ProgramBindingsBase::ResourceAndState::ResourceAndState(Ptr<ResourceBase> resource_ptr, Resource::State state)
    : resource_ptr(std::move(resource_ptr))
    , state(state)
{
    META_FUNCTION_TASK();
}

ProgramBindings::ArgumentBinding::ConstantModificationException::ConstantModificationException(const IProgram::Argument& argument)
    : std::logic_error(fmt::format("Can not modify constant argument binding '{}' of {} shaders.",
                                   argument.GetName(), magic_enum::enum_name(argument.GetShaderType())))
{
    META_FUNCTION_TASK();
}

ProgramBindings::UnboundArgumentsException::UnboundArgumentsException(const IProgram& program, const IProgram::Arguments& unbound_arguments)
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

bool ProgramBindingsBase::ArgumentBindingBase::SetResourceViews(const Resource::Views& resource_views)
{
    META_FUNCTION_TASK();
    if (m_resource_views == resource_views)
        return false;

    if (m_settings.argument.IsConstant() && !m_resource_views.empty())
        throw ConstantModificationException(GetSettings().argument);

    META_CHECK_ARG_NOT_EMPTY_DESCR(resource_views, "can not set empty resources for resource binding");

    const bool        is_addressable_binding = m_settings.argument.IsAddressable();
    const Resource::Type bound_resource_type = m_settings.resource_type;
    META_UNUSED(is_addressable_binding);
    META_UNUSED(bound_resource_type);

    for (const Resource::View& resource_view : resource_views)
    {
        META_CHECK_ARG_NAME_DESCR("resource_view", resource_view.GetResource().GetResourceType() == bound_resource_type,
                                  "incompatible resource type '{}' is bound to argument '{}' of type '{}'",
                                  magic_enum::enum_name(resource_view.GetResource().GetResourceType()),
                                  m_settings.argument.GetName(), magic_enum::enum_name(bound_resource_type));

        const Resource::Usage resource_usage_mask = resource_view.GetResource().GetUsage();
        using namespace magic_enum::bitwise_operators;
        META_CHECK_ARG_DESCR(resource_usage_mask, static_cast<bool>(resource_usage_mask & Resource::Usage::Addressable) == is_addressable_binding,
                             "resource addressable usage flag does not match with resource binding state");
        META_CHECK_ARG_NAME_DESCR("resource_view", is_addressable_binding || !resource_view.GetOffset(),
                                  "can not set resource view_id with non-zero offset to non-addressable resource binding");
    }

    Data::Emitter<ProgramBindings::IArgumentBindingCallback>::Emit(&ProgramBindings::IArgumentBindingCallback::OnProgramArgumentBindingResourceViewsChanged, std::cref(*this), std::cref(m_resource_views), std::cref(resource_views));

    m_resource_views = resource_views;
    return true;
}

ProgramBindingsBase::ArgumentBindingBase::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("{} is bound to {}", m_settings.argument, fmt::join(m_resource_views, ", "));
}

bool ProgramBindingsBase::ArgumentBindingBase::IsAlreadyApplied(const IProgram& program,
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
        previous_argument_argument_binding.GetResourceViews() == m_resource_views)
        return true;

    return false;
}

ProgramBindingsBase::ProgramBindingsBase(const Ptr<IProgram>& program_ptr, const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index)
    : ProgramBindingsBase(program_ptr, frame_index)
{
    META_FUNCTION_TASK();
    SetResourcesForArguments(resource_views_by_argument);
    VerifyAllArgumentsAreBoundToResources();
}

ProgramBindingsBase::ProgramBindingsBase(const ProgramBindingsBase& other_program_bindings, const ResourceViewsByArgument& replace_resource_views_by_argument, const Opt<Data::Index>& frame_index)
    : ProgramBindingsBase(other_program_bindings, frame_index)
{
    META_FUNCTION_TASK();
    SetResourcesForArguments(ReplaceResourceViews(other_program_bindings.GetArgumentBindings(), replace_resource_views_by_argument));
    VerifyAllArgumentsAreBoundToResources();
}

ProgramBindingsBase::ProgramBindingsBase(const Ptr<IProgram>& program_ptr, Data::Index frame_index)
    : m_program_ptr(program_ptr)
    , m_frame_index(frame_index)
    , m_bindings_index(static_cast<ProgramBase&>(*m_program_ptr).GetBindingsCountAndIncrement())
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
    , m_transition_resource_states_by_access(other_program_bindings.m_transition_resource_states_by_access)
    , m_bindings_index(static_cast<ProgramBase&>(*m_program_ptr).GetBindingsCountAndIncrement())
{
    META_FUNCTION_TASK();
    InitializeArgumentBindings(&other_program_bindings);
}

IProgram& ProgramBindingsBase::GetProgram() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_program_ptr);
    return *m_program_ptr;
}

IProgram& ProgramBindingsBase::GetProgram()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_program_ptr);
    return *m_program_ptr;
}

void ProgramBindingsBase::OnProgramArgumentBindingResourceViewsChanged(const ArgumentBinding& argument_binding,
                                                                       const Resource::Views& old_resource_views,
                                                                       const Resource::Views& new_resource_views)
{
    META_FUNCTION_TASK();
    if (!m_resource_state_transition_barriers_ptr)
        return;

    // Find resources that are not used anymore for resource binding
    std::set<Resource*> processed_resources;
    for(const Resource::View& old_resource_view : old_resource_views)
    {
        if (old_resource_view.GetResource().GetResourceType() == Resource::Type::Sampler ||
            processed_resources.count(old_resource_view.GetResourcePtr().get()))
            continue;

        // Check if resource is still used in new resource locations
        if (std::find_if(new_resource_views.begin(), new_resource_views.end(),
                         [&old_resource_view](const Resource::View& new_resource_view)
                         { return new_resource_view.GetResourcePtr() == old_resource_view.GetResourcePtr(); }
                         ) != new_resource_views.end())
        {
            processed_resources.insert(old_resource_view.GetResourcePtr().get());
            continue;
        }

        // Remove unused resources from transition barriers applied for program bindings:
        m_resource_state_transition_barriers_ptr->RemoveStateTransition(old_resource_view.GetResource());
        RemoveTransitionResourceStates(argument_binding, old_resource_view.GetResource());

    }

    for(const Resource::View& new_resource_view : new_resource_views)
    {
        AddTransitionResourceState(argument_binding, new_resource_view.GetResource());
    }
}

void ProgramBindingsBase::InitializeArgumentBindings(const ProgramBindingsBase* other_program_bindings_ptr)
{
    META_FUNCTION_TASK();
    const auto& program = static_cast<const ProgramBase&>(GetProgram());
    const ArgumentBindings& argument_bindings = other_program_bindings_ptr
                                              ? other_program_bindings_ptr->GetArgumentBindings()
                                              : program.GetArgumentBindings();
    for (const auto& [program_argument, argument_binding_ptr] : argument_bindings)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(argument_binding_ptr, "no resource binding is set for program argument '{}'", program_argument.GetName());
        m_arguments.insert(program_argument);
        if (m_binding_by_argument.count(program_argument))
            continue;

        Ptr<ProgramBindingsBase::ArgumentBindingBase> argument_binding_instance_ptr = program.CreateArgumentBindingInstance(argument_binding_ptr, m_frame_index);
        if (argument_binding_ptr->GetSettings().argument.GetAccessorType() == IProgram::ArgumentAccessor::Type::Mutable)
            argument_binding_instance_ptr->Connect(*this);

        m_binding_by_argument.try_emplace(program_argument, std::move(argument_binding_instance_ptr));
    }
}

ProgramBindings::ResourceViewsByArgument ProgramBindingsBase::ReplaceResourceViews(const ArgumentBindings& argument_bindings,
                                                                                   const ResourceViewsByArgument& replace_resource_views) const
{
    META_FUNCTION_TASK();
    ResourceViewsByArgument resource_views_by_argument = replace_resource_views;
    for (const auto& [program_argument, argument_binding_ptr] : argument_bindings)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(argument_binding_ptr, "no resource binding is set for program argument '{}'", program_argument.GetName());

        // NOTE:
        // constant resource bindings are reusing single binding-object for the whole program,
        // so there's no need in setting its value, since it was already set by the original resource binding
        if (argument_binding_ptr->GetSettings().argument.IsConstant() ||
            resource_views_by_argument.count(program_argument))
            continue;

        resource_views_by_argument.try_emplace(program_argument, argument_binding_ptr->GetResourceViews());
    }
    return resource_views_by_argument;
}

void ProgramBindingsBase::SetResourcesForArguments(const ResourceViewsByArgument& resource_views_by_argument)
{
    META_FUNCTION_TASK();
    for (const auto& [program_argument, resource_views] : resource_views_by_argument)
    {
        ProgramBindings::ArgumentBinding& argument_binding = Get(program_argument);
        argument_binding.SetResourceViews(resource_views);
        AddTransitionResourceStates(argument_binding);
    }
    InitResourceRefsByAccess();
}

ProgramBindings::ArgumentBinding& ProgramBindingsBase::Get(const IProgram::Argument& shader_argument) const
{
    META_FUNCTION_TASK();
    const auto binding_by_argument_it = m_binding_by_argument.find(shader_argument);
    if (binding_by_argument_it == m_binding_by_argument.end())
        throw IProgram::Argument::NotFoundException(*m_program_ptr, shader_argument);

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

IProgram::Arguments ProgramBindingsBase::GetUnboundArguments() const
{
    META_FUNCTION_TASK();
    IProgram::Arguments unbound_arguments;
    for (const auto& [program_argument, argument_binding_ptr] : m_binding_by_argument)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(argument_binding_ptr, "no resource binding is set for program argument '{}'", program_argument.GetName());

        if (argument_binding_ptr->GetResourceViews().empty())
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
    if (IProgram::Arguments unbound_arguments = GetUnboundArguments();
        !unbound_arguments.empty())
    {
        throw UnboundArgumentsException(*m_program_ptr, unbound_arguments);
    }
}

void ProgramBindingsBase::ClearTransitionResourceStates()
{
    META_FUNCTION_TASK();
    for(ResourceStates& resource_states : m_transition_resource_states_by_access)
    {
        resource_states.clear();
    }
}

void ProgramBindingsBase::RemoveTransitionResourceStates(const ProgramBindings::ArgumentBinding& argument_binding, const Resource& resource)
{
    META_FUNCTION_TASK();
    if (resource.GetResourceType() == Resource::Type::Sampler)
        return;

    const ProgramBindings::ArgumentBinding::Settings& argument_binding_settings = argument_binding.GetSettings();
    ResourceStates& transition_resource_states = m_transition_resource_states_by_access[argument_binding_settings.argument.GetAccessorIndex()];
    const auto transition_resource_state_it = std::find_if(transition_resource_states.begin(), transition_resource_states.end(),
                                                           [&resource](const ResourceAndState& resource_state)
                                                           { return resource_state.resource_ptr.get() == &resource; });
    if (transition_resource_state_it != transition_resource_states.end())
        transition_resource_states.erase(transition_resource_state_it);
}

void ProgramBindingsBase::AddTransitionResourceState(const ProgramBindings::ArgumentBinding& argument_binding, Resource& resource)
{
    META_FUNCTION_TASK();
    if (resource.GetResourceType() == Resource::Type::Sampler)
        return;

    const ProgramBindings::ArgumentBinding::Settings& argument_binding_settings = argument_binding.GetSettings();
    const Resource::State target_resource_state = GetBoundResourceTargetState(resource, argument_binding_settings.resource_type, argument_binding_settings.argument.IsConstant());
    ResourceStates& transition_resource_states = m_transition_resource_states_by_access[argument_binding_settings.argument.GetAccessorIndex()];
    transition_resource_states.emplace_back(std::dynamic_pointer_cast<ResourceBase>(resource.GetPtr()), target_resource_state);
}

void ProgramBindingsBase::AddTransitionResourceStates(const ProgramBindings::ArgumentBinding& argument_binding)
{
    META_FUNCTION_TASK();
    const ProgramBindings::ArgumentBinding::Settings& argument_binding_settings = argument_binding.GetSettings();
    ResourceStates& transition_resource_states = m_transition_resource_states_by_access[argument_binding_settings.argument.GetAccessorIndex()];

    for(const ResourceView& resource_view : argument_binding.GetResourceViews())
    {
        if (!resource_view.GetResourcePtr())
            continue;

        const Resource& resource = resource_view.GetResource();
        if (resource.GetResourceType() == Resource::Type::Sampler)
            continue;

        const Resource::State target_resource_state = GetBoundResourceTargetState(resource, argument_binding_settings.resource_type, argument_binding_settings.argument.IsConstant());
        transition_resource_states.emplace_back(std::dynamic_pointer_cast<ResourceBase>(resource_view.GetResourcePtr()), target_resource_state);
    }
}

bool ProgramBindingsBase::ApplyResourceStates(IProgram::ArgumentAccessor::Type access_types_mask, const CommandQueue* owner_queue_ptr) const
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    bool                                 resource_states_changed = false;
    for(IProgram::ArgumentAccessor::Type access_type : magic_enum::enum_values<IProgram::ArgumentAccessor::Type>())
    {
        if (!static_cast<bool>(access_types_mask & access_type))
            continue;

        const ResourceStates& resource_states = m_transition_resource_states_by_access[magic_enum::enum_index(access_type).value()];
        for(const ResourceAndState& resource_state : resource_states)
        {
            META_CHECK_ARG_NOT_NULL(resource_state.resource_ptr);
            if (owner_queue_ptr)
                resource_states_changed |= resource_state.resource_ptr->SetOwnerQueueFamily(owner_queue_ptr->GetFamilyIndex(), m_resource_state_transition_barriers_ptr);

            resource_states_changed |= resource_state.resource_ptr->SetState(resource_state.state, m_resource_state_transition_barriers_ptr);
        }
    }

    return resource_states_changed;
}

void ProgramBindingsBase::InitResourceRefsByAccess()
{
    META_FUNCTION_TASK();
    constexpr size_t access_count = magic_enum::enum_count<IProgram::ArgumentAccessor::Type>();
    std::array<std::set<Resource*>, access_count> unique_resources_by_access;

    for (auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        META_CHECK_ARG_NOT_NULL(argument_binding_ptr);
        std::set<Resource*>& unique_resources = unique_resources_by_access[argument_binding_ptr->GetSettings().argument.GetAccessorIndex()];
        for (const Resource::View & resource_view : argument_binding_ptr->GetResourceViews())
        {
            unique_resources.emplace(resource_view.GetResourcePtr().get());
        }
    }

    for(size_t access_index = 0; access_index < access_count; ++access_index)
    {
        const std::set<Resource*>& unique_resources = unique_resources_by_access[access_index];
        Refs<Resource>& resource_refs = m_resource_refs_by_access[access_index];
        resource_refs.clear();
        std::transform(unique_resources.begin(), unique_resources.end(), std::back_inserter(resource_refs),
                       [](Resource* resource_ptr) { return Ref<Resource>(*resource_ptr); });
    }
}

const Refs<Resource>& ProgramBindingsBase::GetResourceRefsByAccess(IProgram::ArgumentAccessor::Type access_type) const
{
    META_FUNCTION_TASK();
    return m_resource_refs_by_access[magic_enum::enum_index(access_type).value()];
}

} // namespace Methane::Graphics
