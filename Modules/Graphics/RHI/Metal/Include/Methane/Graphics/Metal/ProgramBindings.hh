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

FILE: Methane/Graphics/Metal/ProgramBindings.hh
Metal implementation of the program bindings interface.

******************************************************************************/

#pragma once

#include "ProgramArgumentBinding.hh"

#include <Methane/Graphics/Base/ProgramBindings.h>

#import <Metal/Metal.h>

namespace Methane::Graphics::Metal
{

class ProgramBindings final
    : public Base::ProgramBindings
{
public:
    using ArgumentBinding = ProgramArgumentBinding;
    
    ProgramBindings(const Ptr<Rhi::IProgram>& program_ptr, const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index);
    ProgramBindings(const ProgramBindings& other_program_bindings, const ResourceViewsByArgument& replace_resource_view_by_argument, const Opt<Data::Index>& frame_index);

    // IProgramBindings interface
    void Apply(Base::CommandList& command_list, ApplyBehaviorMask apply_behavior) const override;

    // Base::ProgramBindings interface
    void CompleteInitialization() override { }
};

} // namespace Methane::Graphics::Metal
