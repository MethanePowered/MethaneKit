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

FILE: Methane/Graphics/Metal/ProgramBindingsMT.hh
Metal implementation of the program bindings interface.

******************************************************************************/

#pragma once

#include "ProgramArgumentBindingMT.hh"

#include "../../../../Base/Include/Methane/Graphics/ProgramBindingsBase.h"

#import "../../../../../../../../../../../../Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX13.0.sdk/System/Library/Frameworks/Metal.framework/Headers/Metal.h"

namespace Methane::Graphics
{

class ProgramBindingsMT final : public ProgramBindingsBase
{
public:
    using ArgumentBindingMT   = ProgramArgumentBindingMT;
    
    ProgramBindingsMT(const Ptr<IProgram>& program_ptr, const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index);
    ProgramBindingsMT(const ProgramBindingsMT& other_program_bindings, const ResourceViewsByArgument& replace_resource_view_by_argument, const Opt<Data::Index>& frame_index);

    // IProgramBindings interface
    void Apply(CommandListBase& command_list, ApplyBehavior apply_behavior) const override;

    // ProgramBindingsBase interface
    void CompleteInitialization() override { }
};

} // namespace Methane::Graphics
