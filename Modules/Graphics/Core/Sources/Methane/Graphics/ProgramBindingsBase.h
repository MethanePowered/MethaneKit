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

FILE: Methane/Graphics/ProgramBindingsBase.h
Base implementation of the program bindings interface.

******************************************************************************/

#pragma once

#include "ObjectBase.h"
#include "ProgramArgumentBindingBase.h"

#include <Methane/Graphics/IProgramBindings.h>
#include <Methane/Graphics/IResource.h>
#include <Methane/Data/Emitter.hpp>

#include <magic_enum.hpp>

namespace Methane::Graphics
{

class CommandListBase;
class ResourceBase;

class ProgramBindingsBase
    : public IProgramBindings
    , public ObjectBase
    , public Data::Receiver<IProgramBindings::IArgumentBindingCallback>
{
public:
    using ArgumentBindingBase = ProgramArgumentBindingBase;
    using ArgumentBindings = std::unordered_map<IProgram::Argument, Ptr<ArgumentBindingBase>, IProgram::Argument::Hash>;

    ProgramBindingsBase(const Ptr<IProgram>& program_ptr, const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index);
    ProgramBindingsBase(const ProgramBindingsBase& other_program_bindings, const ResourceViewsByArgument& replace_resource_view_by_argument, const Opt<Data::Index>& frame_index);
    ProgramBindingsBase(const Ptr<IProgram>& program_ptr, Data::Index frame_index);
    ProgramBindingsBase(const ProgramBindingsBase& other_program_bindings, const Opt<Data::Index>& frame_index);
    ProgramBindingsBase(ProgramBindingsBase&&) noexcept = default;

    ProgramBindingsBase& operator=(const ProgramBindingsBase& other) = delete;
    ProgramBindingsBase& operator=(ProgramBindingsBase&& other) = delete;

    // IProgramBindings interface
    IProgram&                  GetProgram() const final;
    const IProgram::Arguments& GetArguments() const noexcept final     { return m_arguments; }
    Data::Index                GetFrameIndex() const noexcept final    { return m_frame_index; }
    Data::Index                GetBindingsIndex() const noexcept final { return m_bindings_index; }
    IArgumentBinding&          Get(const IProgram::Argument& shader_argument) const final;
    explicit operator std::string() const final;

    // ProgramBindingsBase interface
    virtual void CompleteInitialization() = 0;
    virtual void Apply(CommandListBase& command_list, ApplyBehavior apply_behavior = ApplyBehavior::AllIncremental) const = 0;

    IProgram::Arguments GetUnboundArguments() const;

    template<typename CommandListType>
    void ApplyResourceTransitionBarriers(CommandListType& command_list,
                                         ProgramArgumentAccessor::Type apply_access_mask = static_cast<ProgramArgumentAccessor::Type>(~0U),
                                         const ICommandQueue* owner_queue_ptr = nullptr) const
    {
        if (ApplyResourceStates(apply_access_mask, owner_queue_ptr) &&
            m_resource_state_transition_barriers_ptr && !m_resource_state_transition_barriers_ptr->IsEmpty())
        {
            command_list.SetResourceBarriers(*m_resource_state_transition_barriers_ptr);
        }
    }

protected:
    // IProgramBindings::IProgramArgumentBindingCallback
    void OnProgramArgumentBindingResourceViewsChanged(const IArgumentBinding&, const IResource::Views&, const IResource::Views&) override;

    void SetResourcesForArguments(const ResourceViewsByArgument& resource_views_by_argument);

    IProgram& GetProgram();
    void InitializeArgumentBindings(const ProgramBindingsBase* other_program_bindings_ptr = nullptr);
    ResourceViewsByArgument ReplaceResourceViews(const ArgumentBindings& argument_bindings,
                                                 const ResourceViewsByArgument& replace_resource_views) const;
    void VerifyAllArgumentsAreBoundToResources() const;
    const ArgumentBindings& GetArgumentBindings() const { return m_binding_by_argument; }
    const Refs<IResource>& GetResourceRefsByAccess(ProgramArgumentAccessor::Type access_type) const;

    void ClearTransitionResourceStates();
    void RemoveTransitionResourceStates(const IProgramBindings::IArgumentBinding& argument_binding, const IResource& resource);
    void AddTransitionResourceState(const IProgramBindings::IArgumentBinding& argument_binding, IResource& resource);
    void AddTransitionResourceStates(const IProgramBindings::IArgumentBinding& argument_binding);

private:
    struct ResourceAndState
    {
        Ptr<ResourceBase> resource_ptr;
        IResource::State  state;

        ResourceAndState(Ptr<ResourceBase> resource_ptr, IResource::State);
    };

    using ResourceStates = std::vector<ResourceAndState>;
    using ResourceStatesByAccess = std::array<ResourceStates, magic_enum::enum_count<ProgramArgumentAccessor::Type>()>;
    using ResourceRefsByAccess = std::array<Refs<IResource>, magic_enum::enum_count<ProgramArgumentAccessor::Type>()>;

    bool ApplyResourceStates(ProgramArgumentAccessor::Type access_types_mask, const ICommandQueue* owner_queue_ptr = nullptr) const;
    void InitResourceRefsByAccess();

    const Ptr<IProgram>             m_program_ptr;
    Data::Index                     m_frame_index;
    IProgram::Arguments             m_arguments;
    ArgumentBindings                m_binding_by_argument;
    ResourceStatesByAccess          m_transition_resource_states_by_access;
    ResourceRefsByAccess            m_resource_refs_by_access;
    mutable Ptr<IResourceBarriers>  m_resource_state_transition_barriers_ptr;
    Data::Index                     m_bindings_index = 0u; // index of this program bindings object between all program bindings of the program
};

} // namespace Methane::Graphics
