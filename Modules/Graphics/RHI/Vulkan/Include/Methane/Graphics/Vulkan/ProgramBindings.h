/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/ProgramBindings.h
Vulkan implementation of the program bindings interface.

******************************************************************************/

#pragma once

#include "ProgramArgumentBinding.h"

#include <Methane/Graphics/Base/ProgramBindings.h>
#include <Methane/Data/Receiver.hpp>

#include <vulkan/vulkan.hpp>
#include <vector>

namespace Methane::Graphics::Vulkan
{

struct ICommandList;
class Program;

class ProgramBindings final
    : public Base::ProgramBindings
    , private Data::Receiver<Rhi::IObjectCallback>
{
public:
    using ArgumentBinding = ProgramArgumentBinding;

    ProgramBindings(Program& program, const BindingValueByArgument& binding_value_by_argument, Data::Index frame_index);
    ProgramBindings(const ProgramBindings& other_program_bindings, const BindingValueByArgument& replace_resource_view_by_argument, const Opt<Data::Index>& frame_index);

    // IProgramBindings interface
    [[nodiscard]] Ptr<Rhi::IProgramBindings> CreateCopy(const BindingValueByArgument& replace_binding_value_by_argument, const Opt<Data::Index>& frame_index) override;
    void Apply(Base::CommandList& command_list, ApplyBehaviorMask apply_behavior) const override;

    // Base::ProgramBindings interface
    void CompleteInitialization() override;

    void Apply(ICommandList& command_list, const Rhi::ICommandQueue& command_queue,
               const Base::ProgramBindings* applied_program_bindings_ptr, ApplyBehaviorMask apply_behavior) const;

protected:
    // IProgramBindings::IProgramArgumentBindingCallback
    void OnProgramArgumentBindingResourceViewsChanged(const IArgumentBinding&,
                                                      const Rhi::ResourceViews&,
                                                      const Rhi::ResourceViews&) override;

private:
    // IObjectCallback interface
    void OnObjectNameChanged(Rhi::IObject&, const std::string&) override; // IProgram name changed

    void SetResourcesForArguments(const BindingValueByArgument& binding_value_by_argument);

    template<typename FuncType> // function void(const ProgramArgument&, ArgumentBinding&)
    void ForEachArgumentBinding(FuncType argument_binding_function) const;
    void UpdateDynamicDescriptorOffsets();
    void UpdateMutableDescriptorSetName();

    mutable Ptr<Rhi::IResourceBarriers> m_resource_ownership_transition_barriers_ptr;
    std::vector<vk::DescriptorSet>      m_descriptor_sets; // descriptor sets corresponding to pipeline layout in the order of their access type
    bool                                m_has_mutable_descriptor_set = false; // if true, then m_descriptor_sets.back() is mutable descriptor set
    std::vector<uint32_t>               m_dynamic_offsets; // dynamic buffer offsets for all descriptor sets from the bound ResourceView::Settings::offset
    std::vector<uint32_t>               m_dynamic_offset_index_by_set_index; // beginning index in dynamic buffer offsets corresponding to the particular descriptor set or access type
};

} // namespace Methane::Graphics::Vulkan
