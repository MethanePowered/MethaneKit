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

FILE: Methane/Graphics/ProgramArgumentBindingBase.h
Base implementation of the program argument binding interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/IProgramBindings.h>
#include <Methane/Graphics/IResource.h>
#include <Methane/Data/Emitter.hpp>

namespace Methane::Graphics
{

class ContextBase;
class ProgramBindingsBase;

class ProgramArgumentBindingBase
    : public IProgramArgumentBinding
    , public Data::Emitter<IProgramBindings::IArgumentBindingCallback>
    , public std::enable_shared_from_this<ProgramArgumentBindingBase>
{
public:
    static Ptr<ProgramArgumentBindingBase> CreateCopy(const ProgramArgumentBindingBase& other_argument_binding);

    ProgramArgumentBindingBase(const ContextBase& context, const Settings& settings);

    virtual void MergeSettings(const ProgramArgumentBindingBase& other);

    // IArgumentBinding interface
    const Settings&         GetSettings() const noexcept override     { return m_settings; }
    const IResource::Views& GetResourceViews() const noexcept final   { return m_resource_views; }
    bool                    SetResourceViews(const IResource::Views& resource_views) override;
    explicit operator std::string() const final;

    Ptr<ProgramArgumentBindingBase> GetPtr() { return shared_from_this(); }

    bool IsAlreadyApplied(const IProgram& program,
                          const ProgramBindingsBase& applied_program_bindings,
                          bool check_binding_value_changes = true) const;

protected:
    const ContextBase& GetContext() const noexcept { return m_context; }

private:
    const ContextBase& m_context;
    const Settings   m_settings;
    IResource::Views m_resource_views;
};

} // namespace Methane::Graphics
