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

#include <Methane/Graphics/ProgramBindings.h>
#include <Methane/Graphics/Resource.h>
#include <Methane/Data/Emitter.hpp>

#include "CommandListBase.h"
#include "ObjectBase.h"

#include <magic_enum.hpp>
#include <optional>

namespace Methane::Graphics
{

class ContextBase;
class CommandListBase;
class ResourceBase;

class ProgramBindingsBase
    : public ProgramBindings
    , public ObjectBase
    , public Data::Receiver<ProgramBindings::IArgumentBindingCallback>
{
public:
    class ArgumentBindingBase
        : public ArgumentBinding
        , public Data::Emitter<ProgramBindings::IArgumentBindingCallback>
        , public std::enable_shared_from_this<ArgumentBindingBase>
    {
    public:
        static Ptr<ArgumentBindingBase> CreateCopy(const ArgumentBindingBase& other_argument_binding);

        ArgumentBindingBase(const ContextBase& context, const Settings& settings);

        virtual void MergeSettings(const ArgumentBindingBase& other);

        // ArgumentBinding interface
        const Settings&            GetSettings() const noexcept override         { return m_settings; }
        const Resource::Locations& GetResourceLocations() const noexcept final   { return m_resource_locations; }
        void                       SetResourceLocations(const Resource::Locations& resource_locations) override;
        explicit operator std::string() const final;

        Ptr<ArgumentBindingBase>   GetPtr() { return shared_from_this(); }

        bool IsAlreadyApplied(const Program& program,
                              const ProgramBindingsBase& applied_program_bindings,
                              bool check_binding_value_changes = true) const;

    protected:
        const ContextBase& GetContext() const noexcept { return m_context; }

    private:
        const ContextBase&  m_context;
        const Settings      m_settings;
        Resource::Locations m_resource_locations;
    };

    using ArgumentBindings = std::unordered_map<Program::Argument, Ptr<ArgumentBindingBase>, Program::Argument::Hash>;

    ProgramBindingsBase(const Ptr<Program>& program_ptr, const ResourceLocationsByArgument& resource_locations_by_argument, Data::Index frame_index);
    ProgramBindingsBase(const ProgramBindingsBase& other_program_bindings, const ResourceLocationsByArgument& replace_resource_location_by_argument, const Opt<Data::Index>& frame_index);
    ProgramBindingsBase(const Ptr<Program>& program_ptr, Data::Index frame_index);
    ProgramBindingsBase(const ProgramBindingsBase& other_program_bindings, const Opt<Data::Index>& frame_index);
    ProgramBindingsBase(ProgramBindingsBase&&) noexcept = default;

    ProgramBindingsBase& operator=(const ProgramBindingsBase& other) = delete;
    ProgramBindingsBase& operator=(ProgramBindingsBase&& other) = delete;

    const Program::Arguments& GetArguments() const noexcept  { return m_arguments; }
    Data::Index               GetFrameIndex() const noexcept { return m_frame_index; }

    // ProgramBindings interface
    Program&         GetProgram() const override;
    ArgumentBinding& Get(const Program::Argument& shader_argument) const override;
    explicit operator std::string() const override;

    // ProgramBindingsBase interface
    virtual void CompleteInitialization() = 0;
    virtual void Apply(CommandListBase& command_list, ApplyBehavior apply_behavior = ApplyBehavior::AllIncremental) const = 0;

    Program::Arguments GetUnboundArguments() const;

protected:
    // ProgramBindings::IArgumentBindingCallback
    void OnProgramArgumentBindingResourceLocationsChanged(const ArgumentBinding&, const Resource::Locations&, const Resource::Locations&) override;

    Program& GetProgram();
    void InitializeArgumentBindings();
    ResourceLocationsByArgument ReplaceResourceLocations(const ArgumentBindings& argument_bindings,
                                                         const ResourceLocationsByArgument& replace_resource_locations);
    void SetResourcesForArguments(const ResourceLocationsByArgument& resource_locations_by_argument);
    void VerifyAllArgumentsAreBoundToResources() const;
    const ArgumentBindings& GetArgumentBindings() const { return m_binding_by_argument; }

    void ClearTransitionResourceStates();
    void RemoveTransitionResourceStates(const ProgramBindings::ArgumentBinding& argument_binding, const Resource& resource);
    void AddTransitionResourceState(const ProgramBindings::ArgumentBinding& argument_binding, Resource& resource);
    void AddTransitionResourceStates(const ProgramBindings::ArgumentBinding& argument_binding);

    template<typename CommandListType>
    void ApplyResourceTransitionBarriers(CommandListType& command_list, Program::ArgumentAccessor::Type apply_access_mask) const
    {
        if (ApplyResourceStates(apply_access_mask) &&
            m_resource_transition_barriers_ptr && !m_resource_transition_barriers_ptr->IsEmpty())
        {
            command_list.SetResourceBarriers(*m_resource_transition_barriers_ptr);
        }
    }

private:
    struct ResourceAndState
    {
        Ptr<ResourceBase> resource_ptr;
        Resource::State   state;

        ResourceAndState(Ptr<ResourceBase> resource_ptr, Resource::State);
    };

    using ResourceStates = std::vector<ResourceAndState>;
    using ResourceStatesByAccess = std::array<ResourceStates, magic_enum::enum_count<Program::ArgumentAccessor::Type>()>;

    bool ApplyResourceStates(Program::ArgumentAccessor::Type access_types_mask) const;

    const Ptr<Program>              m_program_ptr;
    Data::Index                     m_frame_index;
    Program::Arguments              m_arguments;
    ArgumentBindings                m_binding_by_argument;
    ResourceStatesByAccess          m_transition_resource_states_by_access;
    mutable Ptr<Resource::Barriers> m_resource_transition_barriers_ptr;
};

} // namespace Methane::Graphics
