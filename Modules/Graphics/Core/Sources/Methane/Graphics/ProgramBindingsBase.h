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

FILE: Methane/Graphics/ProgramBindingsBase.h
Base implementation of the program bindings interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ProgramBindings.h>
#include <Methane/Graphics/Resource.h>

#include "DescriptorHeap.h"
#include "CommandListBase.h"

#include <optional>

namespace Methane::Graphics
{

class ContextBase;

class ProgramBindingsBase
    : public ProgramBindings
    , public std::enable_shared_from_this<ProgramBindingsBase>
{
public:
    class ArgumentBindingBase
        : public ArgumentBinding
        , public std::enable_shared_from_this<ArgumentBindingBase>
    {
    public:
        struct Settings
        {
            Shader::Type   shader_type;
            std::string    argument_name;
            Resource::Type resource_type;
            uint32_t       resource_count;
            bool           is_constant;
            bool           is_addressable;
        };

        ArgumentBindingBase(ContextBase& context, const Settings& settings);
        ArgumentBindingBase(const ArgumentBindingBase& other) = default;

        // ResourceBinding interface
        Shader::Type               GetShaderType() const override           { return m_settings.shader_type; }
        const std::string&         GetArgumentName() const override         { return m_settings.argument_name; }
        bool                       IsConstant() const override              { return m_settings.is_constant; }
        bool                       IsAddressable() const override           { return m_settings.is_addressable; }
        uint32_t                   GetResourceCount() const override        { return m_settings.resource_count; }
        const Resource::Locations& GetResourceLocations() const override    { return m_resource_locations; }
        void                       SetResourceLocations(const Resource::Locations& resource_locations) override;

        DescriptorHeap::Type       GetDescriptorHeapType() const;
        Ptr<ArgumentBindingBase>   GetPtr() { return shared_from_this(); }
        bool HasResources() const { return !m_resource_locations.empty(); }
        bool IsAlreadyApplied(const Program& program, const Program::Argument& program_argument,
                              const CommandListBase::CommandState& command_state,
                              bool check_binding_value_changes) const;

    protected:
        ContextBase&        m_context;
        const Settings      m_settings;
        Resource::Locations m_resource_locations;
    };

    ProgramBindingsBase(const Ptr<Program>& sp_program, const ResourceLocationsByArgument& resource_locations_by_argument);
    ProgramBindingsBase(const ProgramBindingsBase& other_resource_bingings, const ResourceLocationsByArgument& replace_resource_location_by_argument);
    ~ProgramBindingsBase() override;

    Ptr<ProgramBindingsBase>  GetPtr()              { return shared_from_this(); }
    const Program::Arguments& GetArguments() const  { return m_arguments; }
    const Program&            GetProgram() const    { return *m_sp_program; }

    // ResourceBindings interface
    const Ptr<ArgumentBinding>& Get(const Program::Argument& shader_argument) const override;

    // ProgramBindingsBase interface
    virtual void CompleteInitialization() = 0;

    bool AllArgumentsAreBoundToResources(std::string& missing_args) const;

protected:
    void ReserveDescriptorHeapRanges();
    void SetResourcesForArguments(const ResourceLocationsByArgument& resource_locations_by_argument);
    void VerifyAllArgumentsAreBoundToResources();

    using BindingByArgument = std::unordered_map<Program::Argument, Ptr<ArgumentBindingBase>, Program::Argument::Hash>;
    using DescriptorHeapReservationByType = std::array<std::optional<DescriptorHeap::Reservation>,
                                                       static_cast<uint32_t>(DescriptorHeap::Type::Count)>;

    const Ptr<Program>              m_sp_program;
    Program::Arguments              m_arguments;
    BindingByArgument               m_resource_binding_by_argument;
    DescriptorHeapReservationByType m_descriptor_heap_reservations_by_type;
};

} // namespace Methane::Graphics
