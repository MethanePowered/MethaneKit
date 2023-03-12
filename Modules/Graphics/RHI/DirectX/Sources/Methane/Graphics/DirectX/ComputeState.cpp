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

FILE: Methane/Graphics/DirectX/ComputeState.cpp
DirectX 12 implementation of the render state interface.

******************************************************************************/

#include <Methane/Graphics/DirectX/ComputeState.h>
#include <Methane/Graphics/DirectX/RenderContext.h>
#include <Methane/Graphics/DirectX/ComputeContext.h>
#include <Methane/Graphics/DirectX/Device.h>
#include <Methane/Graphics/DirectX/Program.h>
#include <Methane/Graphics/DirectX/Shader.h>
#include <Methane/Graphics/DirectX/Types.h>
#include <Methane/Graphics/DirectX/ComputeCommandList.h>
#include <Methane/Graphics/DirectX/ErrorHandling.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <directx/d3dx12_core.h>

namespace Methane::Graphics::DirectX
{

static const Device& GetDirectDeviceFromContext(const Rhi::IContext& context)
{
    META_FUNCTION_TASK();
    switch (context.GetType())
    {
    case Rhi::ContextType::Render:
        return dynamic_cast<const RenderContext&>(context).GetDirectDevice();

    case Rhi::ContextType::Compute:
        return dynamic_cast<const ComputeContext&>(context).GetDirectDevice();

    default:
        META_UNEXPECTED_ARG_DESCR(context.GetType(), "Unexpected context type");
    }
}

[[nodiscard]]
inline CD3DX12_SHADER_BYTECODE GetShaderByteCode(const Ptr<Rhi::IShader>& shader_ptr)
{
    META_FUNCTION_TASK();
    const Data::Chunk* p_byte_code_chunk = shader_ptr ? static_cast<const Shader&>(*shader_ptr).GetNativeByteCode() : nullptr;
    return p_byte_code_chunk
        ? CD3DX12_SHADER_BYTECODE(p_byte_code_chunk->GetDataPtr(), p_byte_code_chunk->GetDataSize())
        : CD3DX12_SHADER_BYTECODE(nullptr, 0);
}

ComputeState::ComputeState(const Rhi::IContext& context, const Settings& settings)
    : Base::ComputeState(context, settings)
    , m_device(GetDirectDeviceFromContext(context))
{
    META_FUNCTION_TASK();
    Reset(settings); // NOSONAR - method is not overridable in final class
}

void ComputeState::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    Base::ComputeState::Reset(settings);

    // Set pipeline state descriptor for program
    Program& dx_program = GetDirectProgram();
    m_pipeline_state_desc.pRootSignature = dx_program.GetNativeRootSignature().Get();
    m_pipeline_state_desc.CS             = GetShaderByteCode(dx_program.GetShader(Rhi::ShaderType::Compute));
    m_pipeline_state_desc.NodeMask       = {};
    m_pipeline_state_desc.CachedPSO      = {};
    m_pipeline_state_desc.Flags          = {};

    m_cp_pipeline_state.Reset();
}

void ComputeState::Apply(Base::ComputeCommandList& command_list)
{
    META_FUNCTION_TASK();
    auto& dx_compute_command_list = static_cast<ComputeCommandList&>(command_list);
    ID3D12GraphicsCommandList& d3d12_command_list = dx_compute_command_list.GetNativeCommandList();

    d3d12_command_list.SetPipelineState(GetNativePipelineState().Get());
    d3d12_command_list.SetComputeRootSignature(GetDirectProgram().GetNativeRootSignature().Get());
}

bool ComputeState::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Base::ComputeState::SetName(name))
        return false;

    if (m_cp_pipeline_state)
    {
        m_cp_pipeline_state->SetName(nowide::widen(name).c_str());
    }
    return true;
}

void ComputeState::InitializeNativePipelineState()
{
    META_FUNCTION_TASK();
    if (m_cp_pipeline_state)
        return;

    const wrl::ComPtr<ID3D12Device>& cp_native_device = GetDirectDevice().GetNativeDevice();
    ThrowIfFailed(cp_native_device->CreateComputePipelineState(&m_pipeline_state_desc, IID_PPV_ARGS(&m_cp_pipeline_state)), cp_native_device.Get());
    SetName(GetName());
}

wrl::ComPtr<ID3D12PipelineState>& ComputeState::GetNativePipelineState()
{
    META_FUNCTION_TASK();
    if (!m_cp_pipeline_state)
    {
        InitializeNativePipelineState();
    }
    return m_cp_pipeline_state;
}

Program& ComputeState::GetDirectProgram()
{
    META_FUNCTION_TASK();
    return static_cast<Program&>(GetProgram());
}

} // namespace Methane::Graphics::DirectX
