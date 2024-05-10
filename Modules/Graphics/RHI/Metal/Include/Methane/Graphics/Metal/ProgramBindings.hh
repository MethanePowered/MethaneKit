/******************************************************************************

Copyright 2019-2024 Evgeny Gorodetskiy

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

#include <map>
#include <set>

namespace Methane::Graphics::Metal
{

class Program;
class RenderCommandList;
class ComputeCommandList;
class DescriptorManager;

//#define METAL_USE_ALL_RESOURCES

class ProgramBindings final
    : public Base::ProgramBindings
{
public:
    using ArgumentBinding = ProgramArgumentBinding;
    using ArgumentsRange = Data::Range<Data::Index>;

    ProgramBindings(Program& program, const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index);
    ProgramBindings(const ProgramBindings& other_program_bindings, const ResourceViewsByArgument& replace_resource_view_by_argument, const Opt<Data::Index>& frame_index);
    ~ProgramBindings() override;

    // IProgramBindings interface
    [[nodiscard]] Ptr<Rhi::IProgramBindings> CreateCopy(const ResourceViewsByArgument& replace_resource_views_by_argument, const Opt<Data::Index>& frame_index) override;
    void Apply(Base::CommandList& command_list, ApplyBehaviorMask apply_behavior) const override;

    // Base::ProgramBindings interface
    void CompleteInitialization() override;

    void SetMutableArgumentsRange(const ArgumentsRange& mutable_arg_range);
    const ArgumentsRange& GetMutableArgumentsRange() const { return m_mutable_argument_buffer_range; }

    bool IsUsingNativeResource(__unsafe_unretained id<MTLResource> mtl_resource) const;
    Program& GetMetalProgram() const;

private:
    bool WriteArgumentsBufferRange(DescriptorManager& descriptor_manager,
                                   Rhi::ProgramArgumentAccessType access_type,
                                   const ArgumentsRange& args_range);

    const ArgumentsRange& GetArgumentsRange(Rhi::ProgramArgumentAccessType access_type) const;

    template<typename FuncType> // function void(const ArgumentBinding&)
    void ForEachChangedArgumentBinding(const Base::ProgramBindings* applied_program_bindings_ptr,
                                       ApplyBehaviorMask apply_behavior,
                                       FuncType functor) const;

    void SetRenderResources(const id<MTLRenderCommandEncoder>& mtl_cmd_encoder,
                            const Base::ProgramBindings* applied_program_bindings_ptr,
                            ApplyBehaviorMask apply_behavior) const;
    void SetComputeResources(const id<MTLComputeCommandEncoder>& mtl_cmd_encoder,
                             const Base::ProgramBindings* applied_program_bindings_ptr,
                             ApplyBehaviorMask apply_behavior) const;

    void SetRenderArgumentBuffers(const id<MTLRenderCommandEncoder>& mtl_cmd_encoder) const;
    void SetComputeArgumentBuffers(const id<MTLComputeCommandEncoder>& mtl_cmd_encoder) const;

    using NativeResourceUsageAndStage = std::pair<MTLResourceUsage, MTLRenderStages>;
    using NativeResourcesByUsage      = std::map<NativeResourceUsageAndStage, ArgumentBinding::NativeResources>;
    using NativeResourceSet           = std::set<__unsafe_unretained id<MTLResource>>;
    void UpdateUsedResources();
    NativeResourcesByUsage GetChangedResourcesByUsage(const Base::ProgramBindings* applied_program_bindings_ptr) const;

    void UseRenderResources(const id<MTLRenderCommandEncoder>& mtl_cmd_encoder,
                            const Base::ProgramBindings* applied_program_bindings_ptr) const;
    void UseComputeResources(const id<MTLComputeCommandEncoder>& mtl_cmd_encoder,
                            const Base::ProgramBindings* applied_program_bindings_ptr) const;

    void Apply(RenderCommandList& argument_binding, ApplyBehaviorMask apply_behavior) const;
    void Apply(ComputeCommandList& compute_command_list, ApplyBehaviorMask apply_behavior) const;

    // IProgramBindings::IProgramArgumentBindingCallback
    void OnProgramArgumentBindingResourceViewsChanged(const IArgumentBinding&, const Rhi::IResource::Views&, const Rhi::IResource::Views&) override;

    bool              m_argument_buffers_initialized = false;
    ArgumentsRange    m_mutable_argument_buffer_range;
    NativeResourceSet m_mtl_used_resources;
};

} // namespace Methane::Graphics::Metal
