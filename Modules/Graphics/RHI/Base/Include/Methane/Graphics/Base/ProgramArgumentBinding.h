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

FILE: Methane/Graphics/Base/ProgramArgumentBinding.h
Base implementation of the program argument binding interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RHI/IProgramBindings.h>
#include <Methane/Graphics/RHI/IResource.h>
#include <Methane/Data/Emitter.hpp>

namespace Methane::Graphics::Base
{

class Context;
class ProgramBindings;

class ProgramArgumentBinding
    : public Rhi::IProgramArgumentBinding
    , public Data::Emitter<Rhi::IProgramArgumentBindingCallback>
    , public std::enable_shared_from_this<ProgramArgumentBinding>
{
public:
    ProgramArgumentBinding(const Context& context, const Settings& settings);

    // Base::ProgramArgumentBinding interface
    [[nodiscard]] virtual Ptr<ProgramArgumentBinding> CreateCopy() const = 0;
    virtual void MergeSettings(const ProgramArgumentBinding& other);

    // IArgumentBinding interface
    const Settings&           GetSettings() const noexcept override     { return m_settings; }
    const Rhi::ResourceViews& GetResourceViews() const noexcept final   { return m_resource_views; }
    bool                      SetResourceViews(const Rhi::ResourceViews& resource_views) override;
    explicit operator std::string() const final;

    Ptr<ProgramArgumentBinding> GetPtr() { return shared_from_this(); }

    bool IsAlreadyApplied(const Rhi::IProgram& program,
                          const ProgramBindings& applied_program_bindings,
                          bool check_binding_value_changes = true) const;

protected:
    const Context& GetContext() const noexcept { return m_context; }

private:
    const Context&     m_context;
    const Settings     m_settings;
    Rhi::ResourceViews m_resource_views;
};

} // namespace Methane::Graphics::Base
