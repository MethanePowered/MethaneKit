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

FILE: Methane/Graphics/Base/ProgramBindings.h
Base implementation of the program bindings interface.

******************************************************************************/

#pragma once

#include "Object.h"
#include "ProgramArgumentBinding.h"

#include <Methane/Graphics/RHI/IProgramBindings.h>
#include <Methane/Graphics/RHI/IResource.h>
#include <Methane/Data/Emitter.hpp>

#include <magic_enum.hpp>

namespace Methane::Graphics::Base
{

class CommandList;
class Resource;
class Program;

class ProgramBindings
    : public Rhi::IProgramBindings
    , public Object
    , public Data::Receiver<Rhi::IProgramBindings::IArgumentBindingCallback>
{
public:
    using ArgumentBinding  = ProgramArgumentBinding;
    using ArgumentBindings = std::unordered_map<Rhi::ProgramArgument, Ptr<ArgumentBinding>, Rhi::IProgram::Argument::Hash>;

    ProgramBindings(Program& program, Data::Index frame_index);
    ProgramBindings(Program& program, const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index);
    ProgramBindings(const ProgramBindings& other_program_bindings, const ResourceViewsByArgument& replace_resource_view_by_argument, const Opt<Data::Index>& frame_index);
    ProgramBindings(const ProgramBindings& other_program_bindings, const Opt<Data::Index>& frame_index);
    ProgramBindings(ProgramBindings&&) noexcept = default;

    ProgramBindings& operator=(const ProgramBindings& other) = delete;
    ProgramBindings& operator=(ProgramBindings&& other) = delete;

    // IProgramBindings interface
    Rhi::IProgram&                  GetProgram() const final;
    const Rhi::IProgram::Arguments& GetArguments() const noexcept final     { return m_arguments; }
    Data::Index                     GetFrameIndex() const noexcept final    { return m_frame_index; }
    Data::Index                     GetBindingsIndex() const noexcept final { return m_bindings_index; }
    IArgumentBinding&               Get(const Rhi::IProgram::Argument& shader_argument) const final;
    explicit operator std::string() const final;

    // ProgramBindings interface
    virtual void Initialize();
    virtual void CompleteInitialization() = 0;
    virtual void Apply(CommandList& command_list, ApplyBehaviorMask apply_behavior = ApplyBehaviorMask(~0U)) const = 0;

    Rhi::IProgram::Arguments GetUnboundArguments() const;

    template<typename CommandListType>
    void ApplyResourceTransitionBarriers(CommandListType& command_list,
                                         Rhi::ProgramArgumentAccessMask apply_access = Rhi::ProgramArgumentAccessMask{ ~0U },
                                         const Rhi::ICommandQueue* owner_queue_ptr = nullptr) const
    {
        if (ApplyResourceStates(apply_access, owner_queue_ptr) &&
            m_resource_state_transition_barriers_ptr && !m_resource_state_transition_barriers_ptr->IsEmpty())
        {
            command_list.SetResourceBarriers(*m_resource_state_transition_barriers_ptr);
        }
    }

protected:
    // IProgramBindings::IProgramArgumentBindingCallback
    void OnProgramArgumentBindingResourceViewsChanged(const IArgumentBinding&, const Rhi::IResource::Views&, const Rhi::IResource::Views&) override;

    void RemoveFromDescriptorManager();
    void SetResourcesForArguments(const ResourceViewsByArgument& resource_views_by_argument);
    void InitializeArgumentBindings(const ProgramBindings* other_program_bindings_ptr = nullptr);
    ResourceViewsByArgument ReplaceResourceViews(const ArgumentBindings& argument_bindings,
                                                 const ResourceViewsByArgument& replace_resource_views) const;
    void VerifyAllArgumentsAreBoundToResources() const;
    const ArgumentBindings& GetArgumentBindings() const { return m_binding_by_argument; }
    const Refs<Rhi::IResource>& GetResourceRefsByAccess(Rhi::ProgramArgumentAccessType access_type) const;

    void ClearTransitionResourceStates();
    void RemoveTransitionResourceStates(const Rhi::IProgramBindings::IArgumentBinding& argument_binding, const Rhi::IResource& resource);
    void AddTransitionResourceState(const Rhi::IProgramBindings::IArgumentBinding& argument_binding, Rhi::IResource& resource);
    void AddTransitionResourceStates(const Rhi::IProgramBindings::IArgumentBinding& argument_binding);

private:
    struct ResourceAndState
    {
        Ptr<Resource> resource_ptr;
        Rhi::ResourceState  state;

        ResourceAndState(Ptr<Resource> resource_ptr, Rhi::ResourceState);
    };

    using ResourceStates = std::vector<ResourceAndState>;
    using ResourceStatesByAccess = std::array<ResourceStates, magic_enum::enum_count<Rhi::ProgramArgumentAccessType>()>;
    using ResourceRefsByAccess = std::array<Refs<Rhi::IResource>, magic_enum::enum_count<Rhi::ProgramArgumentAccessType>()>;

    bool ApplyResourceStates(Rhi::ProgramArgumentAccessMask access, const Rhi::ICommandQueue* owner_queue_ptr = nullptr) const;
    void InitResourceRefsByAccess();

    const Ptr<Rhi::IProgram>             m_program_ptr;
    Data::Index                          m_frame_index;
    Rhi::IProgram::Arguments             m_arguments;
    ArgumentBindings                     m_binding_by_argument;
    ResourceStatesByAccess               m_transition_resource_states_by_access;
    ResourceRefsByAccess                 m_resource_refs_by_access;
    mutable Ptr<Rhi::IResourceBarriers>  m_resource_state_transition_barriers_ptr;
    Data::Index                          m_bindings_index = 0u; // index of this program bindings object between all program bindings of the program
};

} // namespace Methane::Graphics::Base
