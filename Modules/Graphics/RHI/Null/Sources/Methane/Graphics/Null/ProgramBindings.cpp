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

#include <Methane/Graphics/Null/ProgramBindings.h>
#include <Methane/Graphics/Null/Program.h>
#include <Methane/Graphics/Null/Device.h>

namespace Methane::Graphics::Null
{

Ptr<Rhi::IProgramBindings> ProgramBindings::CreateCopy(const BindingValueByArgument& replace_binding_value_by_argument,
                                                       const Opt<Data::Index>& frame_index)
{
    META_FUNCTION_TASK();
    return std::make_shared<ProgramBindings>(*this, replace_binding_value_by_argument, frame_index);
}

void ProgramBindings::Apply(Base::CommandList& command_list, ApplyBehaviorMask apply_behavior) const
{
    // Set resource transition barriers before applying resource bindings
    if (apply_behavior.HasAnyBit(ApplyBehavior::StateBarriers))
    {
        Rhi::ProgramArgumentAccessMask apply_access(~0U);
        Base::ProgramBindings::ApplyResourceTransitionBarriers(command_list, apply_access, &command_list.GetCommandQueue());
    }
}

} // namespace Methane::Graphics::Null