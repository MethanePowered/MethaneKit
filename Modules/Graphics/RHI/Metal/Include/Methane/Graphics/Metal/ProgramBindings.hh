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
#include <Methane/Data/Range.hpp>

#import <Metal/Metal.h>

namespace Methane::Graphics::Metal
{

class Program;
class RenderCommandList;
class ComputeCommandList;

class ProgramBindings final
    : public Base::ProgramBindings
{
public:
    using ArgumentBinding = ProgramArgumentBinding;
    using ArgumentsRange = Data::Range<Data::Index>;
    
    ProgramBindings(Program& program, const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index);
    ProgramBindings(const ProgramBindings& other_program_bindings, const ResourceViewsByArgument& replace_resource_view_by_argument, const Opt<Data::Index>& frame_index);

    // IProgramBindings interface
    [[nodiscard]] Ptr<Rhi::IProgramBindings> CreateCopy(const ResourceViewsByArgument& replace_resource_views_by_argument, const Opt<Data::Index>& frame_index) override;
    void Apply(Base::CommandList& command_list, ApplyBehaviorMask apply_behavior) const override;

    // Base::ProgramBindings interface
    void CompleteInitialization() override { }

    void CompleteInitialization(const ArgumentsRange& arg_range);
    const ArgumentsRange& GetArgumentsRange() const { return m_argument_buffer_range; }

private:
    template<typename FuncType> // function void(const ArgumentBinding&)
    void ForEachChangedArgumentBinding(const Base::ProgramBindings* applied_program_bindings_ptr, ApplyBehaviorMask apply_behavior, FuncType functor) const;

    void Apply(RenderCommandList& argument_binding, ApplyBehaviorMask apply_behavior) const;
    void Apply(ComputeCommandList& compute_command_list, ApplyBehaviorMask apply_behavior) const;

    ArgumentsRange m_argument_buffer_range;
};

} // namespace Methane::Graphics::Metal
