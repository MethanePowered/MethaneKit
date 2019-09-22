/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/DirectX12/RenderStateDX.cpp
DirectX 12 implementation of the render state interface.

******************************************************************************/

#include "RenderStateDX.h"
#include "ContextDX.h"
#include "DeviceDX.h"
#include "ProgramDX.h"
#include "ShaderDX.h"
#include "TypesDX.h"
#include "TextureDX.h"
#include "RenderCommandListDX.h"

#include <d3dx12.h>
#include <D3Dcompiler.h>
#include <nowide/convert.hpp>
#include <cassert>

#include <Methane/Instrumentation.h>
#include <Methane/Graphics/Windows/Helpers.h>
#include <Methane/Platform/Windows/Utils.h>

namespace Methane::Graphics
{

constexpr size_t g_max_rtv_count = sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC::RTVFormats) / sizeof(DXGI_FORMAT);

inline CD3DX12_SHADER_BYTECODE GetShaderByteCode(const Shader::Ptr& sp_shader)
{
    ITT_FUNCTION_TASK();
    return sp_shader
        ? CD3DX12_SHADER_BYTECODE(static_cast<const ShaderDX&>(*sp_shader).GetNativeByteCode().Get())
        : CD3DX12_SHADER_BYTECODE(NULL, 0);
}

D3D12_FILL_MODE ConvertRasterizerFillModeToD3D12(RenderState::Rasterizer::FillMode fill_mode)
{
    ITT_FUNCTION_TASK();

    using RasterizerFillMode = RenderState::Rasterizer::FillMode;
    
    switch (fill_mode)
    {
    case RasterizerFillMode::Solid:     return D3D12_FILL_MODE_SOLID;
    case RasterizerFillMode::Wireframe: return D3D12_FILL_MODE_WIREFRAME;
    default:                            assert(0);
    }
    return D3D12_FILL_MODE_SOLID;
}


D3D12_CULL_MODE ConvertRasterizerCullModeToD3D12(RenderState::Rasterizer::CullMode cull_mode)
{
    ITT_FUNCTION_TASK();

    using RasterizerCullMode = RenderState::Rasterizer::CullMode;

    switch (cull_mode)
    {
    case RasterizerCullMode::None:      return D3D12_CULL_MODE_NONE;
    case RasterizerCullMode::Front:     return D3D12_CULL_MODE_FRONT;
    case RasterizerCullMode::Back:      return D3D12_CULL_MODE_BACK;
    default:                            assert(0);
    }
    return D3D12_CULL_MODE_NONE;
}

D3D12_STENCIL_OP ConvertStencilOperationToD3D12(RenderState::Stencil::Operation operation)
{
    ITT_FUNCTION_TASK();

    using StencilOperation = RenderState::Stencil::Operation;
    
    switch (operation)
    {
    case StencilOperation::Keep:            return D3D12_STENCIL_OP_KEEP;
    case StencilOperation::Zero:            return D3D12_STENCIL_OP_ZERO;
    case StencilOperation::Replace:         return D3D12_STENCIL_OP_REPLACE;
    case StencilOperation::Invert:          return D3D12_STENCIL_OP_INVERT;
    case StencilOperation::IncrementClamp:  return D3D12_STENCIL_OP_INCR_SAT;
    case StencilOperation::DecrementClamp:  return D3D12_STENCIL_OP_DECR_SAT;
    case StencilOperation::IncrementWrap:   return D3D12_STENCIL_OP_INCR;
    case StencilOperation::DecrementWrap:   return D3D12_STENCIL_OP_DECR;
    default:                                assert(0);
    }
    return D3D12_STENCIL_OP_KEEP;
}

D3D12_DEPTH_STENCILOP_DESC ConvertStencilFaceOperationsToD3D12(const RenderState::Stencil::FaceOperations& stencil_face_op)
{
    ITT_FUNCTION_TASK();

    D3D12_DEPTH_STENCILOP_DESC stencil_desc = { };

    stencil_desc.StencilFailOp      = ConvertStencilOperationToD3D12(stencil_face_op.stencil_failure);
    stencil_desc.StencilPassOp      = ConvertStencilOperationToD3D12(stencil_face_op.stencil_pass);
    stencil_desc.StencilDepthFailOp = ConvertStencilOperationToD3D12(stencil_face_op.depth_failure);
    stencil_desc.StencilFunc        = TypeConverterDX::CompareFunctionToDX(stencil_face_op.compare);

    return stencil_desc;
}

RenderState::Ptr RenderState::Create(Context& context, const RenderState::Settings& state_settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<RenderStateDX>(static_cast<ContextBase&>(context), state_settings);
}

RenderStateDX::RenderStateDX(ContextBase& context, const Settings& settings)
    : RenderStateBase(context, settings)
{
    ITT_FUNCTION_TASK();
    Reset(settings);
}

void RenderStateDX::Reset(const Settings& settings)
{
    ITT_FUNCTION_TASK();
    RenderStateBase::Reset(settings);

    ProgramDX&         dx_program               = RenderStateDX::GetProgramDX();
    Program::Settings  program_settings         = dx_program.GetSettings();

    // Set Rasterizer state descriptor
    CD3DX12_RASTERIZER_DESC                     rasterizer_desc(D3D12_DEFAULT);
    rasterizer_desc.FillMode                    = ConvertRasterizerFillModeToD3D12(m_settings.rasterizer.fill_mode);
    rasterizer_desc.CullMode                    = ConvertRasterizerCullModeToD3D12(m_settings.rasterizer.cull_mode);
    rasterizer_desc.FrontCounterClockwise       = m_settings.rasterizer.is_front_counter_clockwise;
    rasterizer_desc.MultisampleEnable           = m_settings.rasterizer.sample_count > 1;
    rasterizer_desc.ForcedSampleCount           = !m_settings.depth.enabled && !m_settings.stencil.enabled ? m_settings.rasterizer.sample_count : 0;

    CD3DX12_BLEND_DESC                          blend_desc(D3D12_DEFAULT);
    blend_desc.AlphaToCoverageEnable            = m_settings.rasterizer.alpha_to_coverage_enabled;
    blend_desc.IndependentBlendEnable           = FALSE;
    // TODO: impulate setting m_settings.rasterizer.alpha_to_one_enabled

    // Set depth and stencil state descriptor
    CD3DX12_DEPTH_STENCIL_DESC                  depth_stencil_desc(D3D12_DEFAULT);
    depth_stencil_desc.DepthEnable              = m_settings.depth.enabled;
    depth_stencil_desc.DepthWriteMask           = D3D12_DEPTH_WRITE_MASK_ALL;
    depth_stencil_desc.DepthFunc                = TypeConverterDX::CompareFunctionToDX(m_settings.depth.compare);
    depth_stencil_desc.StencilEnable            = m_settings.stencil.enabled;
    depth_stencil_desc.StencilReadMask          = m_settings.stencil.read_mask;
    depth_stencil_desc.StencilWriteMask         = m_settings.stencil.write_mask;
    depth_stencil_desc.FrontFace                = ConvertStencilFaceOperationsToD3D12(m_settings.stencil.front_face);
    depth_stencil_desc.BackFace                 = ConvertStencilFaceOperationsToD3D12(m_settings.stencil.back_face);

    // Set pipeline state descriptor for program
    m_pipeline_state_desc.InputLayout           = dx_program.GetNativeInputLayoutDesc();
    m_pipeline_state_desc.pRootSignature        = dx_program.GetNativeRootSignature().Get();
    m_pipeline_state_desc.VS                    = GetShaderByteCode(dx_program.GetShader(Shader::Type::Vertex));
    m_pipeline_state_desc.PS                    = GetShaderByteCode(dx_program.GetShader(Shader::Type::Pixel));
    m_pipeline_state_desc.DepthStencilState     = depth_stencil_desc;
    m_pipeline_state_desc.BlendState            = blend_desc;
    m_pipeline_state_desc.RasterizerState       = rasterizer_desc;
    m_pipeline_state_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // Not used: for GS or HS shaders only
    m_pipeline_state_desc.SampleMask            = UINT_MAX;
    m_pipeline_state_desc.SampleDesc.Count      = m_settings.rasterizer.sample_count;

    // Set RTV, DSV formats for pipeline state
    if (program_settings.color_formats.size() > g_max_rtv_count)
    {
        throw new std::runtime_error("Number of color attachments (" + std::to_string(program_settings.color_formats.size()) +
                                     ") exceeds maximum RTV count in DirectX (" + std::to_string(g_max_rtv_count) + ")");
    }
    std::fill_n(m_pipeline_state_desc.RTVFormats, g_max_rtv_count, DXGI_FORMAT_UNKNOWN);
    uint32_t attachment_index = 0;
    for (PixelFormat color_format : program_settings.color_formats)
    {
        m_pipeline_state_desc.RTVFormats[attachment_index++] = TypeConverterDX::DataFormatToDXGI(color_format);
    }
    m_pipeline_state_desc.NumRenderTargets = static_cast<UINT>(program_settings.color_formats.size());
    m_pipeline_state_desc.DSVFormat = m_settings.depth.enabled ? TypeConverterDX::DataFormatToDXGI(program_settings.depth_format) : DXGI_FORMAT_UNKNOWN;

    m_viewports     = TypeConverterDX::ViewportsToD3D(m_settings.viewports);
    m_scissor_rects = TypeConverterDX::ScissorRectsToD3D(m_settings.scissor_rects);

    m_cp_pipeline_state.Reset();
}

void RenderStateDX::Apply(RenderCommandListBase& command_list)
{
    ITT_FUNCTION_TASK();

    RenderCommandListDX& dx_command_list = static_cast<RenderCommandListDX&>(command_list);
    wrl::ComPtr<ID3D12GraphicsCommandList>& cp_dx_command_list = dx_command_list.GetNativeCommandList();

    cp_dx_command_list->SetPipelineState(GetNativePipelineState().Get());
    cp_dx_command_list->SetGraphicsRootSignature(GetProgramDX().GetNativeRootSignature().Get());
    cp_dx_command_list->RSSetViewports(static_cast<UINT>(m_viewports.size()), m_viewports.data());
    cp_dx_command_list->RSSetScissorRects(static_cast<UINT>(m_scissor_rects.size()), m_scissor_rects.data());
}

void RenderStateDX::SetViewports(const Viewports& viewports)
{
    ITT_FUNCTION_TASK();
    RenderStateBase::SetViewports(viewports);

    m_viewports = TypeConverterDX::ViewportsToD3D(viewports);
}

void RenderStateDX::SetScissorRects(const ScissorRects& scissor_rects)
{
    ITT_FUNCTION_TASK();

    RenderStateBase::SetScissorRects(scissor_rects);

    m_scissor_rects = TypeConverterDX::ScissorRectsToD3D(scissor_rects);
}

void RenderStateDX::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();
    RenderStateBase::SetName(name);

    if (m_cp_pipeline_state)
    {
        m_cp_pipeline_state->SetName(nowide::widen(name).c_str());
    }
}

wrl::ComPtr<ID3D12PipelineState>& RenderStateDX::GetNativePipelineState()
{
    ITT_FUNCTION_TASK();
    if (!m_cp_pipeline_state)
    {
        ThrowIfFailed(GetContextDX().GetDeviceDX().GetNativeDevice()->CreateGraphicsPipelineState(&m_pipeline_state_desc, IID_PPV_ARGS(&m_cp_pipeline_state)));
        SetName(GetName());
    }
    return m_cp_pipeline_state;
}

ProgramDX& RenderStateDX::GetProgramDX()
{
    ITT_FUNCTION_TASK();
    assert(!!m_settings.sp_program);
    return static_cast<ProgramDX&>(*m_settings.sp_program);
}

ContextDX& RenderStateDX::GetContextDX()
{
    ITT_FUNCTION_TASK();
    return static_cast<class ContextDX&>(m_context);
}

} // namespace Methane::Graphics
