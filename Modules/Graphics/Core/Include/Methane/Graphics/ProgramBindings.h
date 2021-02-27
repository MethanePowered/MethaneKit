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

FILE: Methane/Graphics/ProgramBindings.h
Methane program bindings interface for resources binding to program arguments.

******************************************************************************/

#pragma once

#include "Program.h"
#include "Resource.h"

#include <Methane/Memory.hpp>

#include <string>
#include <unordered_map>
#include <stdexcept>

namespace Methane::Graphics
{

struct ProgramBindings
{
    struct ArgumentBinding
    {
        // ArgumentBinding settings
        struct Settings
        {
            Program::ArgumentAccessor argument;
            Resource::Type            resource_type;
            uint32_t              resource_count = 1;
        };

        class ConstantModificationException : public std::logic_error
        {
        public:
            ConstantModificationException();
        };

        // ArgumentBinding interface
        [[nodiscard]] virtual const Settings&            GetSettings() const noexcept = 0;
        [[nodiscard]] virtual const Resource::Locations& GetResourceLocations() const noexcept = 0;
        virtual void SetResourceLocations(const Resource::Locations& resource_locations) = 0;

        virtual ~ArgumentBinding() = default;
    };

    using ArgumentBindings = std::unordered_map<Program::Argument, Ptr<ProgramBindings::ArgumentBinding>, Program::Argument::Hash>;

    enum class ApplyBehavior : uint32_t
    {
        Indifferent    = 0U,        // All bindings will be applied indifferently of the previous binding values
        ConstantOnce   = 1U << 0,   // Constant program arguments will be applied only once for each command list
        ChangesOnly    = 1U << 1,   // Only changed program argument values will be applied in command sequence
        StateBarriers  = 1U << 2,   // Resource state barriers will be automatically evaluated and set for command list
        AllIncremental = ~0U        // All binding values will be applied incrementally along with resource state barriers
    };

    using ResourceLocationsByArgument = std::unordered_map<Program::Argument, Resource::Locations, Program::Argument::Hash>;

    class UnboundArgumentsException: public std::runtime_error
    {
    public:
        UnboundArgumentsException(const Program& program, const Program::Arguments& unbound_arguments);

        [[nodiscard]] const Program& GetProgram() const noexcept { return m_program; }
        [[nodiscard]] const Program::Arguments& GetArguments() const noexcept { return m_unbound_arguments; }

    private:
        const Program& m_program;
        const Program::Arguments m_unbound_arguments;
    };

    // Create ProgramBindings instance
    [[nodiscard]] static Ptr<ProgramBindings> Create(const Ptr<Program>& program_ptr, const ResourceLocationsByArgument& resource_locations_by_argument);
    [[nodiscard]] static Ptr<ProgramBindings> CreateCopy(const ProgramBindings& other_program_bindings, const ResourceLocationsByArgument& replace_resource_locations_by_argument = {});

    // ProgramBindings interface
    [[nodiscard]] virtual const Ptr<ArgumentBinding>& Get(const Program::Argument& shader_argument) const = 0;

    virtual ~ProgramBindings() = default;
};

} // namespace Methane::Graphics
