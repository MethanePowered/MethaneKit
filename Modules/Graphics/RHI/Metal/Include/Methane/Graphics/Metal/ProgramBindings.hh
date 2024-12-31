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

#include <Methane/Graphics/RHI/ICommandList.h>
#include <Methane/Graphics/Base/ProgramBindings.h>
#include <Methane/Data/Range.hpp>
#include <Methane/Instrumentation.h>

#import <Metal/Metal.h>

#include <map>
#include <set>
#include <mutex>

namespace Methane::Graphics::Metal
{

class Program;
class RenderCommandList;
class ComputeCommandList;
class DescriptorManager;

//#define METAL_USE_ALL_RESOURCES

template<Rhi::CommandListType command_type> struct Command
{
    using EncoderType = void;
    using RhiListType = void;
};

template<> struct Command<Rhi::CommandListType::Render>
{
    using EncoderType = __unsafe_unretained id<MTLRenderCommandEncoder>;
    using RhiListType = RenderCommandList;
};

template<> struct Command<Rhi::CommandListType::Compute>
{
    using EncoderType = __unsafe_unretained id<MTLComputeCommandEncoder>;
    using RhiListType = ComputeCommandList;
};

class ProgramBindings final
    : public Base::ProgramBindings
{
public:
    using ArgumentBinding = ProgramArgumentBinding;
    using ArgumentsRange  = Data::Range<Data::Index>;
    using CommandType     = Rhi::CommandListType;

    ProgramBindings(Program& program, const BindingValueByArgument& binding_value_by_argument, Data::Index frame_index);
    ProgramBindings(const ProgramBindings& other_program_bindings, const BindingValueByArgument& replace_resource_view_by_argument, const Opt<Data::Index>& frame_index);
    ~ProgramBindings() override;

    // IProgramBindings interface
    [[nodiscard]] Ptr<Rhi::IProgramBindings> CreateCopy(const BindingValueByArgument& replace_binding_value_by_argument, const Opt<Data::Index>& frame_index) override;
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

    template<typename FuncType> // function void(const ProgramArgument&, ArgumentBinding&)
    void ForEachArgumentBinding(FuncType argument_binding_function) const;

    template<typename FuncType> // function void(const ArgumentBinding&)
    void ForEachChangedArgumentBinding(const Base::ProgramBindings* applied_program_bindings_ptr,
                                       ApplyBehaviorMask apply_behavior,
                                       FuncType functor) const;

    template<CommandType command_type, typename CommandEncoderType = typename Command<command_type>::EncoderType>
    void SetMetalResources(const CommandEncoderType& mtl_cmd_encoder,
                           const Base::ProgramBindings* applied_program_bindings_ptr,
                           ApplyBehaviorMask apply_behavior) const;

    template<CommandType command_type, typename CommandEncoderType = typename Command<command_type>::EncoderType>
    void SetMetalArgumentBuffers(const CommandEncoderType& mtl_cmd_encoder,
                                 const Base::ProgramBindings* applied_program_bindings_ptr,
                                 ApplyBehaviorMask apply_behavior) const;

    template<CommandType command_type, typename CommandEncoderType = typename Command<command_type>::EncoderType>
    void UseMetalResources(const CommandEncoderType& mtl_cmd_encoder,
                           const Base::ProgramBindings* applied_program_bindings_ptr) const;

    using NativeResourceUsageAndStage = std::pair<MTLResourceUsage, MTLRenderStages>;
    using NativeResourcesByUsage      = std::map<NativeResourceUsageAndStage, ArgumentBinding::NativeResources>;
    using NativeResourceSet           = std::set<__unsafe_unretained id<MTLResource>>;
    NativeResourcesByUsage GetChangedResourcesByUsage(const Base::ProgramBindings* applied_program_bindings_ptr) const;
    void UpdateUsedResources();
    void UpdateArgumentBuffer(const IArgumentBinding& changed_arg_binding);

    template<CommandType command_type, typename CommandListType = typename Command<command_type>::RhiListType>
    void Apply(CommandListType& command_list, ApplyBehaviorMask apply_behavior) const;

    // IProgramArgumentBindingCallback
    void OnProgramArgumentBindingResourceViewsChanged(const IArgumentBinding&, const Rhi::ResourceViews&, const Rhi::ResourceViews&) override;
    void OnProgramArgumentBindingRootConstantChanged(const IArgumentBinding&, const Rhi::RootConstant&) override;

    using AccessTypeMask = Data::EnumMask<Rhi::ProgramArgumentAccessType>;

    AccessTypeMask            m_argument_buffer_initialized_access_types;
    ArgumentsRange            m_mutable_argument_buffer_range;
    bool                      m_has_root_constant_values = false;
    NativeResourceSet         m_mtl_used_resources;
    TracyLockable(std::mutex, m_used_resources_mutex);
};

} // namespace Methane::Graphics::Metal
