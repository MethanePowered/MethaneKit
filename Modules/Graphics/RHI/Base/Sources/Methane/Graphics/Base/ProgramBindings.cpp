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

FILE: Methane/Graphics/Base/ProgramBindings.cpp
Base implementation of the program bindings interface.

******************************************************************************/

#include <Methane/Graphics/Base/ProgramBindings.h>
#include <Methane/Graphics/Base/Program.h>
#include <Methane/Graphics/Base/Resource.h>
#include <Methane/Graphics/Base/CommandList.h>

#include <Methane/Graphics/RHI/IBuffer.h>
#include <Methane/Graphics/RHI/ITexture.h>
#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Platform/Utils.h>

#include <magic_enum.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <array>

namespace Methane::Graphics::Base
{

static Rhi::ResourceState GetBoundResourceTargetState(const Rhi::IResource& resource, Rhi::IResource::Type resource_type, bool is_constant_binding)
{
    META_FUNCTION_TASK();
    switch (resource_type)
    {
    case Rhi::IResource::Type::Buffer:
        // FIXME: state transition of DX upload heap resources should be reworked properly and made friendly with Vulkan
        // DX resource in upload heap can not be transitioned to any other state but initial GenericRead state
        if (dynamic_cast<const Rhi::IBuffer&>(resource).GetSettings().storage_mode != Rhi::IBuffer::StorageMode::Private)
            return resource.GetState();
        else if (is_constant_binding)
            return Rhi::ResourceState::ConstantBuffer;
        break;

    case Rhi::IResource::Type::Texture:
        if (dynamic_cast<const Rhi::ITexture&>(resource).GetSettings().type == Rhi::ITexture::Type::DepthStencilBuffer)
            return Rhi::ResourceState::DepthRead;
        break;

    default:
        break;
    }
    return Rhi::ResourceState::ShaderResource;
}

ProgramBindings::ResourceAndState::ResourceAndState(Ptr<Resource> resource_ptr, Rhi::ResourceState state)
    : resource_ptr(std::move(resource_ptr))
    , state(state)
{
    META_FUNCTION_TASK();
}

ProgramBindings::ProgramBindings(const Ptr<Rhi::IProgram>& program_ptr, const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index)
    : ProgramBindings(program_ptr, frame_index)
{
    META_FUNCTION_TASK();
    SetResourcesForArguments(resource_views_by_argument);
    VerifyAllArgumentsAreBoundToResources();
}

ProgramBindings::ProgramBindings(const ProgramBindings& other_program_bindings, const ResourceViewsByArgument& replace_resource_views_by_argument, const Opt<Data::Index>& frame_index)
    : ProgramBindings(other_program_bindings, frame_index)
{
    META_FUNCTION_TASK();
    SetResourcesForArguments(ReplaceResourceViews(other_program_bindings.GetArgumentBindings(), replace_resource_views_by_argument));
    VerifyAllArgumentsAreBoundToResources();
}

ProgramBindings::ProgramBindings(const Ptr<Rhi::IProgram>& program_ptr, Data::Index frame_index)
    : m_program_ptr(program_ptr)
    , m_frame_index(frame_index)
    , m_bindings_index(static_cast<Program&>(*m_program_ptr).GetBindingsCountAndIncrement())
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_ZERO(program_ptr);
    InitializeArgumentBindings();
}

ProgramBindings::ProgramBindings(const ProgramBindings& other_program_bindings, const Opt<Data::Index>& frame_index)
    : Object(other_program_bindings)
    , Data::Receiver<IProgramBindings::IArgumentBindingCallback>()
    , m_program_ptr(other_program_bindings.m_program_ptr)
    , m_frame_index(frame_index.value_or(other_program_bindings.m_frame_index))
    , m_transition_resource_states_by_access(other_program_bindings.m_transition_resource_states_by_access)
    , m_bindings_index(static_cast<Program&>(*m_program_ptr).GetBindingsCountAndIncrement())
{
    META_FUNCTION_TASK();
    InitializeArgumentBindings(&other_program_bindings);
}

Rhi::IProgram& ProgramBindings::GetProgram() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_program_ptr);
    return *m_program_ptr;
}

Rhi::IProgram& ProgramBindings::GetProgram()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_program_ptr);
    return *m_program_ptr;
}

void ProgramBindings::OnProgramArgumentBindingResourceViewsChanged(const IArgumentBinding& argument_binding,
                                                                       const Rhi::IResource::Views& old_resource_views,
                                                                       const Rhi::IResource::Views& new_resource_views)
{
    META_FUNCTION_TASK();
    if (!m_resource_state_transition_barriers_ptr)
        return;

    // Find resources that are not used anymore for resource binding
    std::set<Rhi::IResource*> processed_resources;
    for(const Rhi::IResource::View& old_resource_view : old_resource_views)
    {
        if (old_resource_view.GetResource().GetResourceType() == Rhi::IResource::Type::Sampler ||
            processed_resources.count(old_resource_view.GetResourcePtr().get()))
            continue;

        // Check if resource is still used in new resource locations
        if (std::find_if(new_resource_views.begin(), new_resource_views.end(),
                         [&old_resource_view](const Rhi::IResource::View& new_resource_view)
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

    for(const Rhi::IResource::View& new_resource_view : new_resource_views)
    {
        AddTransitionResourceState(argument_binding, new_resource_view.GetResource());
    }
}

void ProgramBindings::InitializeArgumentBindings(const ProgramBindings* other_program_bindings_ptr)
{
    META_FUNCTION_TASK();
    const auto& program = static_cast<const Program&>(GetProgram());
    const ArgumentBindings& argument_bindings = other_program_bindings_ptr
                                              ? other_program_bindings_ptr->GetArgumentBindings()
                                              : program.GetArgumentBindings();
    for (const auto& [program_argument, argument_binding_ptr] : argument_bindings)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(argument_binding_ptr, "no resource binding is set for program argument '{}'", program_argument.GetName());
        m_arguments.insert(program_argument);
        if (m_binding_by_argument.count(program_argument))
            continue;

        Ptr<ProgramBindings::ArgumentBinding> argument_binding_instance_ptr = program.CreateArgumentBindingInstance(argument_binding_ptr, m_frame_index);
        if (argument_binding_ptr->GetSettings().argument.GetAccessorType() == Rhi::ProgramArgumentAccess::Type::Mutable)
            argument_binding_instance_ptr->Connect(*this);

        m_binding_by_argument.try_emplace(program_argument, std::move(argument_binding_instance_ptr));
    }
}

Rhi::IProgramBindings::ResourceViewsByArgument ProgramBindings::ReplaceResourceViews(const ArgumentBindings& argument_bindings,
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

void ProgramBindings::SetResourcesForArguments(const ResourceViewsByArgument& resource_views_by_argument)
{
    META_FUNCTION_TASK();
    for (const auto& [program_argument, resource_views] : resource_views_by_argument)
    {
        Rhi::IProgramBindings::IArgumentBinding& argument_binding = Get(program_argument);
        argument_binding.SetResourceViews(resource_views);
        AddTransitionResourceStates(argument_binding);
    }
    InitResourceRefsByAccess();
}

Rhi::IProgramBindings::IArgumentBinding& ProgramBindings::Get(const Rhi::IProgram::Argument& shader_argument) const
{
    META_FUNCTION_TASK();
    const auto binding_by_argument_it = m_binding_by_argument.find(shader_argument);
    if (binding_by_argument_it == m_binding_by_argument.end())
        throw Rhi::IProgram::Argument::NotFoundException(*m_program_ptr, shader_argument);

    return *binding_by_argument_it->second;
}

ProgramBindings::operator std::string() const
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

Rhi::IProgram::Arguments ProgramBindings::GetUnboundArguments() const
{
    META_FUNCTION_TASK();
    Rhi::IProgram::Arguments unbound_arguments;
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

void ProgramBindings::VerifyAllArgumentsAreBoundToResources() const
{
    META_FUNCTION_TASK();
    // Verify that resources are set for all program arguments
    if (Rhi::IProgram::Arguments unbound_arguments = GetUnboundArguments();
        !unbound_arguments.empty())
    {
        throw UnboundArgumentsException(*m_program_ptr, unbound_arguments);
    }
}

void ProgramBindings::ClearTransitionResourceStates()
{
    META_FUNCTION_TASK();
    for(ResourceStates& resource_states : m_transition_resource_states_by_access)
    {
        resource_states.clear();
    }
}

void ProgramBindings::RemoveTransitionResourceStates(const Rhi::IProgramBindings::IArgumentBinding& argument_binding, const Rhi::IResource& resource)
{
    META_FUNCTION_TASK();
    if (resource.GetResourceType() == Rhi::IResource::Type::Sampler)
        return;

    const Rhi::IProgramBindings::IArgumentBinding::Settings& argument_binding_settings  = argument_binding.GetSettings();
    ResourceStates                                    & transition_resource_states = m_transition_resource_states_by_access[argument_binding_settings.argument.GetAccessorIndex()];
    const auto transition_resource_state_it = std::find_if(transition_resource_states.begin(), transition_resource_states.end(),
                                                           [&resource](const ResourceAndState& resource_state)
                                                           { return resource_state.resource_ptr.get() == &resource; });
    if (transition_resource_state_it != transition_resource_states.end())
        transition_resource_states.erase(transition_resource_state_it);
}

void ProgramBindings::AddTransitionResourceState(const Rhi::IProgramBindings::IArgumentBinding& argument_binding, Rhi::IResource& resource)
{
    META_FUNCTION_TASK();
    if (resource.GetResourceType() == Rhi::IResource::Type::Sampler)
        return;

    const Rhi::IProgramBindings::IArgumentBinding::Settings& argument_binding_settings = argument_binding.GetSettings();
    const Rhi::ResourceState target_resource_state = GetBoundResourceTargetState(resource, argument_binding_settings.resource_type, argument_binding_settings.argument.IsConstant());
    ResourceStates& transition_resource_states = m_transition_resource_states_by_access[argument_binding_settings.argument.GetAccessorIndex()];
    transition_resource_states.emplace_back(std::dynamic_pointer_cast<Resource>(resource.GetPtr()), target_resource_state);
}

void ProgramBindings::AddTransitionResourceStates(const Rhi::IProgramBindings::IArgumentBinding& argument_binding)
{
    META_FUNCTION_TASK();
    const Rhi::IProgramBindings::IArgumentBinding::Settings& argument_binding_settings  = argument_binding.GetSettings();
    ResourceStates                                    & transition_resource_states = m_transition_resource_states_by_access[argument_binding_settings.argument.GetAccessorIndex()];

    for(const Rhi::ResourceView& resource_view : argument_binding.GetResourceViews())
    {
        if (!resource_view.GetResourcePtr())
            continue;

        const Rhi::IResource& resource = resource_view.GetResource();
        if (resource.GetResourceType() == Rhi::IResource::Type::Sampler)
            continue;

        const Rhi::ResourceState target_resource_state = GetBoundResourceTargetState(resource, argument_binding_settings.resource_type, argument_binding_settings.argument.IsConstant());
        transition_resource_states.emplace_back(std::dynamic_pointer_cast<Resource>(resource_view.GetResourcePtr()), target_resource_state);
    }
}

bool ProgramBindings::ApplyResourceStates(Rhi::ProgramArgumentAccess access, const Rhi::ICommandQueue* owner_queue_ptr) const
{
    META_FUNCTION_TASK();
    bool resource_states_changed = false;

    for(Rhi::ProgramArgumentAccess::Type access_type : access.GetTypes())
    {
        using namespace magic_enum::bitwise_operators;
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

void ProgramBindings::InitResourceRefsByAccess()
{
    META_FUNCTION_TASK();
    constexpr size_t                               access_count = magic_enum::enum_count<Rhi::ProgramArgumentAccess::Type>();
    std::array<std::set<Rhi::IResource*>, access_count> unique_resources_by_access;

    for (auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        META_CHECK_ARG_NOT_NULL(argument_binding_ptr);
        std::set<Rhi::IResource*>      & unique_resources = unique_resources_by_access[argument_binding_ptr->GetSettings().argument.GetAccessorIndex()];
        for (const Rhi::IResource::View& resource_view : argument_binding_ptr->GetResourceViews())
        {
            unique_resources.emplace(resource_view.GetResourcePtr().get());
        }
    }

    for(size_t access_index = 0; access_index < access_count; ++access_index)
    {
        const std::set<Rhi::IResource*>& unique_resources = unique_resources_by_access[access_index];
        Refs<Rhi::IResource>& resource_refs = m_resource_refs_by_access[access_index];
        resource_refs.clear();
        std::transform(unique_resources.begin(), unique_resources.end(), std::back_inserter(resource_refs),
                       [](Rhi::IResource* resource_ptr) { return Ref<Rhi::IResource>(*resource_ptr); });
    }
}

const Refs<Rhi::IResource>& ProgramBindings::GetResourceRefsByAccess(Rhi::ProgramArgumentAccess::Type access_type) const
{
    META_FUNCTION_TASK();
    return m_resource_refs_by_access[magic_enum::enum_index(access_type).value()];
}

} // namespace Methane::Graphics::Base
