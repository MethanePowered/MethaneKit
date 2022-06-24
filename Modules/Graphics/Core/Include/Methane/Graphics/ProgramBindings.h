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
#include "Object.h"

#include <Methane/Data/IEmitter.h>
#include <Methane/Memory.hpp>

#include <string>
#include <unordered_map>
#include <stdexcept>

namespace Methane::Graphics
{

struct ProgramBindings : virtual Object // NOSONAR
{
    struct ArgumentBinding;

    struct IArgumentBindingCallback
    {
        virtual void OnProgramArgumentBindingResourceViewsChanged(const ArgumentBinding& argument_binding, const Resource::Views& old_resource_views, const Resource::Views& new_resource_views) = 0;

        virtual ~IArgumentBindingCallback() = default;
    };

    struct ArgumentBinding
        : virtual Data::IEmitter<IArgumentBindingCallback> // NOSONAR
    {
        struct Settings
        {
            Program::ArgumentAccessor argument;
            Resource::Type            resource_type;
            uint32_t                  resource_count = 1;
        };

        class ConstantModificationException : public std::logic_error
        {
        public:
            explicit ConstantModificationException(const Program::Argument& argument);
        };

        // ArgumentBinding interface
        [[nodiscard]] virtual const Settings&            GetSettings() const noexcept = 0;
        [[nodiscard]] virtual const Resource::Views& GetResourceViews() const noexcept = 0;
        virtual bool SetResourceViews(const Resource::Views& resource_views) = 0;
        [[nodiscard]] virtual explicit operator std::string() const = 0;
    };
    
    enum class ApplyBehavior : uint32_t
    {
        Indifferent     = 0U,        // All bindings will be applied indifferently of the previous binding values
        ConstantOnce    = 1U << 0,   // Constant program arguments will be applied only once for each command list
        ChangesOnly     = 1U << 1,   // Only changed program argument values will be applied in command sequence
        StateBarriers   = 1U << 2,   // Resource state barriers will be automatically evaluated and set for command list
        RetainResources = 1U << 3,   // Retain bound resources in command list state until it is completed on GPU
        AllIncremental  = ~0U        // All binding values will be applied incrementally along with resource state barriers
    };

    using ResourceViewsByArgument = std::unordered_map<Program::Argument, Resource::Views, Program::Argument::Hash>;

    class UnboundArgumentsException: public std::runtime_error
    {
    public:
        UnboundArgumentsException(const Program& program, const Program::Arguments& unbound_arguments);

        [[nodiscard]] const Program&            GetProgram() const noexcept { return m_program; }
        [[nodiscard]] const Program::Arguments& GetArguments() const noexcept { return m_unbound_arguments; }

    private:
        const Program& m_program;
        const Program::Arguments m_unbound_arguments;
    };

    // Create ProgramBindings instance
    [[nodiscard]] static Ptr<ProgramBindings> Create(const Ptr<Program>& program_ptr, const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index = 0U);
    [[nodiscard]] static Ptr<ProgramBindings> CreateCopy(const ProgramBindings& other_program_bindings, const ResourceViewsByArgument& replace_resource_views_by_argument = {}, const Opt<Data::Index>& frame_index = {});

    // ProgramBindings interface
    [[nodiscard]] virtual Program&                  GetProgram() const = 0;
    [[nodiscard]] virtual ArgumentBinding&          Get(const Program::Argument& shader_argument) const = 0;
    [[nodiscard]] virtual const Program::Arguments& GetArguments() const noexcept = 0;
    [[nodiscard]] virtual Data::Index               GetFrameIndex() const noexcept = 0;
    [[nodiscard]] virtual Data::Index               GetBindingsIndex() const noexcept = 0;
    [[nodiscard]] virtual explicit operator         std::string() const = 0;
};

} // namespace Methane::Graphics
