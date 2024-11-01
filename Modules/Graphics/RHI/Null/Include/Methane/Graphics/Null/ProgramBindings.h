/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Null/ProgramBindings.h
Null implementation of the program bindings interface.

******************************************************************************/

#pragma once

#include "ProgramArgumentBinding.h"

#include <Methane/Graphics/RHI/ICommandList.h>
#include <Methane/Graphics/Base/ProgramBindings.h>

namespace Methane::Graphics::Null
{

class ProgramBindings final
    : public Base::ProgramBindings
{
public:
    using ArgumentBinding = ProgramArgumentBinding;

    using Base::ProgramBindings::ProgramBindings;

    // IProgramBindings interface
    [[nodiscard]] Ptr<Rhi::IProgramBindings> CreateCopy(const BindingValueByArgument& replace_binding_value_by_argument, const Opt<Data::Index>& frame_index) override;

    // Base::ProgramBindings overrides...
    void CompleteInitialization() override { /* Intentionally unimplemented */ }
    void Apply(Base::CommandList& command_list, ApplyBehaviorMask apply_behavior) const override;
};

} // namespace Methane::Graphics::Null
