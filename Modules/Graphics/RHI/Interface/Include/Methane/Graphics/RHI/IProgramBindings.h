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

#include "ProgramArgument.h"
#include "IResource.h"
#include "IObject.h"

#include <Methane/Data/IEmitter.h>
#include <Methane/Data/EnumMask.hpp>
#include <Methane/Memory.hpp>

#include <string>
#include <stdexcept>

namespace Methane::Graphics::Rhi
{

struct IProgramArgumentBinding;

struct IProgramArgumentBindingCallback
{
    virtual void OnProgramArgumentBindingResourceViewsChanged(const IProgramArgumentBinding& argument_binding, const ResourceViews& old_resource_views, const ResourceViews& new_resource_views) = 0;
    virtual void OnProgramArgumentBindingRootConstantChanged(const IProgramArgumentBinding& argument_binding, const RootConstant& root_constant) = 0;

    virtual ~IProgramArgumentBindingCallback() = default;
};

class ProgramArgumentConstantModificationException
    : public std::logic_error
{
public:
    explicit ProgramArgumentConstantModificationException(const ProgramArgument& argument);
};

struct ProgramArgumentBindingSettings
{
    ProgramArgumentAccessor argument;
    ResourceType            resource_type;
    uint32_t                resource_count = 1U;
    uint32_t                buffer_size = 0U;
};

struct IProgramArgumentBinding
    : virtual Data::IEmitter<IProgramArgumentBindingCallback> // NOSONAR
{
    using ICallback                     = IProgramArgumentBindingCallback;
    using ConstantModificationException = ProgramArgumentConstantModificationException;
    using Settings                      = ProgramArgumentBindingSettings;

    // IProgramArgumentBinding interface
    [[nodiscard]] virtual const Settings&      GetSettings() const noexcept = 0;
    [[nodiscard]] virtual const ResourceViews& GetResourceViews() const noexcept = 0;
    virtual bool                               SetResourceViewSpan(ResourceViewSpan resource_views) = 0;
    virtual bool                               SetResourceViews(const ResourceViews& resource_views) = 0;
    virtual bool                               SetResourceView(const ResourceView& resource_view) = 0;
    [[nodiscard]] virtual RootConstant         GetRootConstant() const = 0;
    virtual bool                               SetRootConstant(const RootConstant& root_constant) = 0;
    [[nodiscard]] virtual explicit operator std::string() const = 0;
};

enum class ProgramBindingsApplyBehavior : uint32_t
{
    ConstantOnce,
    ChangesOnly,
    StateBarriers,
    RetainResources
};

using ProgramBindingsApplyBehaviorMask = Data::EnumMask<ProgramBindingsApplyBehavior>;

struct IProgram;

class ProgramBindingsUnboundArgumentsException: public std::runtime_error
{
public:
    ProgramBindingsUnboundArgumentsException(const IProgram& program, const ProgramArguments& unbound_arguments);

    [[nodiscard]] const IProgram&         GetProgram() const noexcept   { return m_program; }
    [[nodiscard]] const ProgramArguments& GetArguments() const noexcept { return m_unbound_arguments; }

private:
    const IProgram&        m_program;
    const ProgramArguments m_unbound_arguments;
};

struct IProgramBindings
    : virtual IObject // NOSONAR
{
    using IArgumentBindingCallback  = IProgramArgumentBindingCallback;
    using IArgumentBinding          = IProgramArgumentBinding;
    using BindingValueByArgument    = ProgramBindingValueByArgument;
    using ApplyBehavior             = ProgramBindingsApplyBehavior;
    using ApplyBehaviorMask         = ProgramBindingsApplyBehaviorMask;
    using UnboundArgumentsException = ProgramBindingsUnboundArgumentsException;

    // Create IProgramBindings instance
    [[nodiscard]] static Ptr<IProgramBindings> Create(IProgram& program,
                                                      const BindingValueByArgument& binding_value_by_argument,
                                                      Data::Index frame_index = 0U);

    // IProgramBindings interface
    [[nodiscard]] virtual Ptr<IProgramBindings>   CreateCopy(const BindingValueByArgument& replace_binding_value_by_argument = {},
                                                             const Opt<Data::Index>& frame_index = {}) = 0;
    [[nodiscard]] virtual IProgram&               GetProgram() const = 0;
    [[nodiscard]] virtual IArgumentBinding&       Get(const ProgramArgument& shader_argument) const = 0;
    [[nodiscard]] virtual const ProgramArguments& GetArguments() const noexcept = 0;
    [[nodiscard]] virtual Data::Index             GetFrameIndex() const noexcept = 0;
    [[nodiscard]] virtual Data::Index             GetBindingsIndex() const noexcept = 0;
    [[nodiscard]] virtual explicit operator       std::string() const = 0;
};

} // namespace Methane::Graphics::Rhi
