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

FILE: Methane/Graphics/RHI/IProgramBindings.h
Methane program bindings interface for resources binding to program arguments.

******************************************************************************/

#pragma once

#include "IProgram.h"
#include "IResource.h"
#include "IObject.h"

#include <Methane/Data/IEmitter.h>
#include <Methane/Memory.hpp>

#include <string>
#include <unordered_map>
#include <stdexcept>

namespace Methane::Graphics::Rhi
{

struct IProgramArgumentBinding;

struct IProgramArgumentBindingCallback
{
    virtual void OnProgramArgumentBindingResourceViewsChanged(const IProgramArgumentBinding& argument_binding, const IResource::Views& old_resource_views, const IResource::Views& new_resource_views) = 0;

    virtual ~IProgramArgumentBindingCallback() = default;
};

class ProgramArgumentConstantModificationException
    : public std::logic_error
{
public:
    explicit ProgramArgumentConstantModificationException(const IProgram::Argument& argument);
};

struct ProgramArgumentBindingSettings
{
    Rhi::ProgramArgumentAccessor argument;
    IResource::Type         resource_type;
    uint32_t                resource_count = 1;
};

struct IProgramArgumentBinding
    : virtual Data::IEmitter<IProgramArgumentBindingCallback> // NOSONAR
{
    using ICallback = IProgramArgumentBindingCallback;
    using ConstantModificationException = ProgramArgumentConstantModificationException;
    using Settings = ProgramArgumentBindingSettings;

    // IProgramArgumentBinding interface
    [[nodiscard]] virtual const Settings&         GetSettings() const noexcept = 0;
    [[nodiscard]] virtual const IResource::Views& GetResourceViews() const noexcept = 0;
    virtual bool SetResourceViews(const IResource::Views& resource_views) = 0;
    [[nodiscard]] virtual explicit operator std::string() const = 0;
};

union ProgramBindingsApplyBehavior
{
    enum class Bit
    {
        ConstantOnce,
        ChangesOnly,
        StateBarriers,
        RetainResources
    };

    struct
    {
        bool constant_once    : 1; // Constant program arguments will be applied only once for each command list
        bool changes_only     : 1; // Only changed program argument values will be applied in command sequence
        bool state_barriers   : 1; // Resource state barriers will be automatically evaluated and set for command list
        bool retain_resources : 1; // Retain bound resources in command list state until it is completed on GPU
    };

    // mask =  0: All bindings will be applied indifferently of the previous binding values
    // mask = ~0: All binding values will be applied incrementally along with resource state barriers
    uint32_t mask;

    ProgramBindingsApplyBehavior() noexcept;
    explicit ProgramBindingsApplyBehavior(uint32_t mask) noexcept;
    explicit ProgramBindingsApplyBehavior(const std::initializer_list<Bit>& bits);

    bool operator==(const ProgramBindingsApplyBehavior& other) const noexcept;
    bool operator!=(const ProgramBindingsApplyBehavior& other) const noexcept;

    void SetBit(Bit bit, bool value);
    std::vector<Bit> GetBits() const;
    std::vector<std::string> GetBitNames() const;
};

class ProgramBindingsUnboundArgumentsException: public std::runtime_error
{
public:
    ProgramBindingsUnboundArgumentsException(const IProgram& program, const IProgram::Arguments& unbound_arguments);

    [[nodiscard]] const IProgram&            GetProgram() const noexcept { return m_program; }
    [[nodiscard]] const IProgram::Arguments& GetArguments() const noexcept { return m_unbound_arguments; }

private:
    const IProgram& m_program;
    const IProgram::Arguments m_unbound_arguments;
};

struct IProgramBindings
    : virtual IObject // NOSONAR
{
    using IArgumentBindingCallback = IProgramArgumentBindingCallback;
    using IArgumentBinding = IProgramArgumentBinding;
    using ApplyBehavior = ProgramBindingsApplyBehavior;
    using UnboundArgumentsException = ProgramBindingsUnboundArgumentsException;
    using ResourceViewsByArgument = std::unordered_map<IProgram::Argument, IResource::Views, IProgram::Argument::Hash>;

    // Create IProgramBindings instance
    [[nodiscard]] static Ptr<IProgramBindings> Create(const Ptr<IProgram>& program_ptr, const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index = 0U);
    [[nodiscard]] static Ptr<IProgramBindings> CreateCopy(const IProgramBindings& other_program_bindings, const ResourceViewsByArgument& replace_resource_views_by_argument = {}, const Opt<Data::Index>& frame_index = {});

    // IProgramBindings interface
    [[nodiscard]] virtual IProgram&               GetProgram() const = 0;
    [[nodiscard]] virtual IArgumentBinding&       Get(const ProgramArgument& shader_argument) const = 0;
    [[nodiscard]] virtual const ProgramArguments& GetArguments() const noexcept = 0;
    [[nodiscard]] virtual Data::Index             GetFrameIndex() const noexcept = 0;
    [[nodiscard]] virtual Data::Index             GetBindingsIndex() const noexcept = 0;
    [[nodiscard]] virtual explicit operator       std::string() const = 0;
};

} // namespace Methane::Graphics::Rhi
