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
class CommandListBase;

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
        static Ptr<ArgumentBindingBase> CreateCopy(const ArgumentBindingBase& other_argument_binding);

        ArgumentBindingBase(const ContextBase& context, Settings settings);
        ArgumentBindingBase(const ArgumentBindingBase& other) = default;

        // ArgumentBinding interface
        const Settings&            GetSettings() const noexcept override            { return m_settings; }
        const Resource::Locations& GetResourceLocations() const noexcept override   { return m_resource_locations; }
        void                       SetResourceLocations(const Resource::Locations& resource_locations) override;

        DescriptorHeap::Type       GetDescriptorHeapType() const;
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

    ProgramBindingsBase(const Ptr<Program>& sp_program, const ResourceLocationsByArgument& resource_locations_by_argument);
    ProgramBindingsBase(const ProgramBindingsBase& other_program_bindings, const ResourceLocationsByArgument& replace_resource_location_by_argument);
    ~ProgramBindingsBase() override;

    Ptr<ProgramBindingsBase>  GetPtr()              { return shared_from_this(); }
    const Program::Arguments& GetArguments() const  { return m_arguments; }
    const Program&            GetProgram() const;

    // ProgramBindings interface
    const Ptr<ArgumentBinding>& Get(const Program::Argument& shader_argument) const override;

    // ProgramBindingsBase interface
    virtual void CompleteInitialization() = 0;
    virtual void Apply(CommandListBase& command_list, ApplyBehavior::Mask apply_behavior = ApplyBehavior::AllIncremental) const = 0;

    bool AllArgumentsAreBoundToResources(std::string& missing_args) const;

protected:
    Program& GetProgram();
    void ReserveDescriptorHeapRanges();
    void SetResourcesForArguments(const ResourceLocationsByArgument& resource_locations_by_argument);
    void VerifyAllArgumentsAreBoundToResources();

    using BindingByArgument = std::unordered_map<Program::Argument, Ptr<ArgumentBinding>, Program::Argument::Hash>;
    const BindingByArgument& GetArgumentBindings() const { return m_binding_by_argument; }

    const std::optional<DescriptorHeap::Reservation>& GetDescriptorHeapReservationByType(DescriptorHeap::Type heap_type) const;

private:
    using DescriptorHeapReservationByType = std::array<std::optional<DescriptorHeap::Reservation>,
                                                       static_cast<uint32_t>(DescriptorHeap::Type::Count)>;

    const Ptr<Program>              m_sp_program;
    Program::Arguments              m_arguments;
    BindingByArgument               m_binding_by_argument;
    DescriptorHeapReservationByType m_descriptor_heap_reservations_by_type;
};

} // namespace Methane::Graphics
