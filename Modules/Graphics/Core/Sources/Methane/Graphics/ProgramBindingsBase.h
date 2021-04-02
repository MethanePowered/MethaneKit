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

#include "DescriptorHeap.h"
#include "CommandListBase.h"
#include "ObjectBase.h"

#include <magic_enum.hpp>
#include <optional>

namespace Methane::Graphics
{

class ContextBase;
class CommandListBase;

class ProgramBindingsBase
    : public ProgramBindings
    , public ObjectBase
{
public:
    class ArgumentBindingBase
        : public ArgumentBinding
        , public std::enable_shared_from_this<ArgumentBindingBase>
    {
    public:
        static Ptr<ArgumentBindingBase> CreateCopy(const ArgumentBindingBase& other_argument_binding);

        ArgumentBindingBase(const ContextBase& context, const Settings& settings);
        ArgumentBindingBase(const ArgumentBindingBase& other) = default;

        // ArgumentBinding interface
        const Settings&            GetSettings() const noexcept override            { return m_settings; }
        const Resource::Locations& GetResourceLocations() const noexcept override   { return m_resource_locations; }
        void                       SetResourceLocations(const Resource::Locations& resource_locations) override;
        explicit operator std::string() const override;

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

    using ArgumentBindings = std::unordered_map<Program::Argument, Ptr<ArgumentBindingBase>, Program::Argument::Hash>;

    ProgramBindingsBase(const Ptr<Program>& program_ptr, const ResourceLocationsByArgument& resource_locations_by_argument, Data::Index frame_index = 0U);
    ProgramBindingsBase(const ProgramBindingsBase& other_program_bindings, const ResourceLocationsByArgument& replace_resource_location_by_argument = {}, const Opt<Data::Index>& frame_index = {});
    ProgramBindingsBase(ProgramBindingsBase&&) noexcept = default;
    ~ProgramBindingsBase() override;

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
    Program& GetProgram();
    void ReserveDescriptorHeapRanges();
    void SetResourcesForArguments(const ResourceLocationsByArgument& resource_locations_by_argument) const;
    void VerifyAllArgumentsAreBoundToResources() const;

    const ArgumentBindings& GetArgumentBindings() const { return m_binding_by_argument; }
    const std::optional<DescriptorHeap::Reservation>& GetDescriptorHeapReservationByType(DescriptorHeap::Type heap_type) const;

private:
    using DescriptorHeapReservationByType = std::array<std::optional<DescriptorHeap::Reservation>, magic_enum::enum_count<DescriptorHeap::Type>() - 1>;

    const Ptr<Program>              m_program_ptr;
    Data::Index                     m_frame_index;
    Program::Arguments              m_arguments;
    ArgumentBindings                m_binding_by_argument;
    DescriptorHeapReservationByType m_descriptor_heap_reservations_by_type;
};

class DescriptorsCountByAccess
{
public:
    DescriptorsCountByAccess();

    uint32_t& operator[](Program::ArgumentAccessor::Type access_type);
    uint32_t  operator[](Program::ArgumentAccessor::Type access_type) const;

private:
    std::array<uint32_t, magic_enum::enum_count<Program::ArgumentAccessor::Type>()> m_count_by_access_type;
};

} // namespace Methane::Graphics
