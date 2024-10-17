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

FILE: Methane/Graphics/Metal/ComputeState.mm
Metal implementation of the render state interface.

******************************************************************************/

#include <Methane/Graphics/Metal/ComputeState.hh>
#include <Methane/Graphics/Metal/RenderContext.hh>
#include <Methane/Graphics/Metal/ComputeContext.hh>
#include <Methane/Graphics/Metal/ComputeCommandList.hh>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Metal
{

static const Device& GetMetalDeviceFromContext(const Rhi::IContext& context)
{
    META_FUNCTION_TASK();
    switch (context.GetType())
    {
    case Rhi::ContextType::Render:
        return dynamic_cast<const RenderContext&>(context).GetMetalDevice();

    case Rhi::ContextType::Compute:
        return dynamic_cast<const ComputeContext&>(context).GetMetalDevice();
    }
    return dynamic_cast<const RenderContext&>(context).GetMetalDevice();
}

ComputeState::ComputeState(const Rhi::IContext& context, const Settings& settings)
    : Base::ComputeState(context, settings)
    , m_device(GetMetalDeviceFromContext(context))
{
    META_FUNCTION_TASK();
    Reset(settings);
}

void ComputeState::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    Base::ComputeState::Reset(settings);

    Program& metal_program = static_cast<Program&>(*settings.program_ptr);
    m_mtl_pipeline_state_desc                 = [[MTLComputePipelineDescriptor alloc] init];
    m_mtl_pipeline_state_desc.computeFunction = metal_program.GetNativeShaderFunction(Rhi::ShaderType::Compute);

    ResetNativeState();
}

void ComputeState::Apply(Base::ComputeCommandList& command_list)
{
    META_FUNCTION_TASK();
    auto& metal_command_list = static_cast<ComputeCommandList&>(command_list);
    const id<MTLComputeCommandEncoder>& mtl_cmd_encoder = metal_command_list.GetNativeCommandEncoder();
    [mtl_cmd_encoder setComputePipelineState: GetNativePipelineState()];
}

bool ComputeState::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Base::ComputeState::SetName(name))
        return false;
    
    NSString* ns_name = MacOS::ConvertToNsString(name);
    m_mtl_pipeline_state_desc.label = ns_name;

    ResetNativeState();
    return true;
}

void ComputeState::InitializeNativePipelineState()
{
    META_FUNCTION_TASK();
    if (m_mtl_pipeline_state)
        return;
    
    NSError* ns_error = nil;
    m_mtl_pipeline_state = [m_device.GetNativeDevice() newComputePipelineStateWithDescriptor: m_mtl_pipeline_state_desc
                                                                                     options: MTLPipelineOptionNone
                                                                                  reflection: nil
                                                                                       error: &ns_error];
    META_CHECK_NOT_NULL_DESCR(m_mtl_pipeline_state,
                              "failed to create Metal compute pipeline state: {}",
                              MacOS::ConvertFromNsString([ns_error localizedDescription]));
}

id<MTLComputePipelineState> ComputeState::GetNativePipelineState()
{
    META_FUNCTION_TASK();
    if (!m_mtl_pipeline_state)
    {
        InitializeNativePipelineState();
    }
    return m_mtl_pipeline_state;
}

void ComputeState::ResetNativeState()
{
    META_FUNCTION_TASK();
    m_mtl_pipeline_state = nil;
}

} // namespace Methane::Graphics::Metal
