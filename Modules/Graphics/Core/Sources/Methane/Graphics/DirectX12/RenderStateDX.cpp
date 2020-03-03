/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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
#include "RenderContextDX.h"
#include "DeviceDX.h"
#include "ProgramDX.h"
#include "ShaderDX.h"
#include "TypesDX.h"
#include "TextureDX.h"
#include "RenderCommandListDX.h"

#include <Methane/Graphics/Windows/Helpers.h>
#include <Methane/Platform/Windows/Utils.h>
#include <Methane/Instrumentation.h>

#include <d3dx12.h>
#include <D3Dcompiler.h>
#include <nowide/convert.hpp>
#include <cassert>

namespace Methane::Graphics
{

constexpr size_t g_max_rtv_count = sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC::RTVFormats) / sizeof(DXGI_FORMAT);

inline CD3DX12_SHADER_BYTECODE GetShaderByteCode(const Ptr<Shader>& sp_shader)
{
    ITT_FUNCTION_TASK();
    const Data::Chunk* p_byte_code_chunk = sp_shader ? static_cast<const ShaderDX&>(*sp_shader).GetNativeByteCode() : nullptr;
    return p_byte_code_chunk
        ? CD3DX12_SHADER_BYTECODE(p_byte_code_chunk->p_data, p_byte_code_chunk->size)
        : CD3DX12_SHADER_BYTECODE(NULL, 0);
}

static D3D12_FILL_MODE ConvertRasterizerFillModeToD3D12(RenderState::Rasterizer::FillMode fill_mode)
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


static D3D12_CULL_MODE ConvertRasterizerCullModeToD3D12(RenderState::Rasterizer::CullMode cull_mode)
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

static UINT8 ConvertRenderTargetWriteMaskToD3D12(RenderState::Blending::ColorChannel::Mask rt_write_mask)
{
    ITT_FUNCTION_TASK();

    using ColorChannel = RenderState::Blending::ColorChannel;

    UINT8 d3d12_color_write_mask = 0;
    if (rt_write_mask & ColorChannel::Red)
        d3d12_color_write_mask |= D3D12_COLOR_WRITE_ENABLE_RED;
    if (rt_write_mask & ColorChannel::Green)
        d3d12_color_write_mask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
    if (rt_write_mask & ColorChannel::Blue)
        d3d12_color_write_mask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
    if (rt_write_mask & ColorChannel::Alpha)
        d3d12_color_write_mask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
    return d3d12_color_write_mask;
};

static D3D12_BLEND_OP ConvertBlendingOperationToD3D12(RenderState::Blending::Operation blend_operation)
{
    ITT_FUNCTION_TASK();

    using BlendOp = RenderState::Blending::Operation;

    switch(blend_operation)
    {
    case BlendOp::Add:              return D3D12_BLEND_OP_ADD;
    case BlendOp::Subtract:         return D3D12_BLEND_OP_SUBTRACT;
    case BlendOp::ReverseSubtract:  return D3D12_BLEND_OP_REV_SUBTRACT;
    case BlendOp::Minimum:          return D3D12_BLEND_OP_MIN;
    case BlendOp::Maximum:          return D3D12_BLEND_OP_MAX;
    default:                        assert(0);
    }
    return D3D12_BLEND_OP_ADD;
}

static D3D12_BLEND ConvertBlendingFactorToD3D12(RenderState::Blending::Factor blend_factor)
{
    ITT_FUNCTION_TASK();

    using BlendFactor = RenderState::Blending::Factor;
    
    switch (blend_factor)
    {
    case BlendFactor::Zero:                     return D3D12_BLEND_ZERO;
    case BlendFactor::One:                      return D3D12_BLEND_ONE;
    case BlendFactor::SourceColor:              return D3D12_BLEND_SRC_COLOR;
    case BlendFactor::OneMinusSourceColor:      return D3D12_BLEND_INV_SRC_COLOR;
    case BlendFactor::SourceAlpha:              return D3D12_BLEND_SRC_ALPHA;
    case BlendFactor::OneMinusSourceAlpha:      return D3D12_BLEND_INV_SRC_ALPHA;
    case BlendFactor::DestinationColor:         return D3D12_BLEND_DEST_COLOR;
    case BlendFactor::OneMinusDestinationColor: return D3D12_BLEND_INV_DEST_COLOR;
    case BlendFactor::DestinationAlpha:         return D3D12_BLEND_DEST_ALPHA;
    case BlendFactor::OneMinusDestinationAlpha: return D3D12_BLEND_INV_DEST_ALPHA;
    case BlendFactor::SourceAlphaSaturated:     return D3D12_BLEND_SRC_ALPHA_SAT;
    case BlendFactor::BlendColor:               return D3D12_BLEND_BLEND_FACTOR;
    case BlendFactor::OneMinusBlendColor:       return D3D12_BLEND_INV_BLEND_FACTOR;
    case BlendFactor::BlendAlpha:               return D3D12_BLEND_BLEND_FACTOR;
    case BlendFactor::OneMinusBlendAlpha:       return D3D12_BLEND_INV_BLEND_FACTOR;
    case BlendFactor::Source1Color:             return D3D12_BLEND_SRC1_COLOR;
    case BlendFactor::OneMinusSource1Color:     return D3D12_BLEND_INV_SRC1_COLOR;
    case BlendFactor::Source1Alpha:             return D3D12_BLEND_SRC1_ALPHA;
    case BlendFactor::OneMinusSource1Alpha:     return D3D12_BLEND_INV_SRC1_ALPHA;
    default:                                    assert(0);
    }
    return D3D12_BLEND_ZERO;
}

static D3D12_STENCIL_OP ConvertStencilOperationToD3D12(RenderState::Stencil::Operation operation)
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

static D3D12_DEPTH_STENCILOP_DESC ConvertStencilFaceOperationsToD3D12(const RenderState::Stencil::FaceOperations& stencil_face_op)
{
    ITT_FUNCTION_TASK();

    D3D12_DEPTH_STENCILOP_DESC stencil_desc = { };

    stencil_desc.StencilFailOp      = ConvertStencilOperationToD3D12(stencil_face_op.stencil_failure);
    stencil_desc.StencilPassOp      = ConvertStencilOperationToD3D12(stencil_face_op.stencil_pass);
    stencil_desc.StencilDepthFailOp = ConvertStencilOperationToD3D12(stencil_face_op.depth_failure);
    stencil_desc.StencilFunc        = TypeConverterDX::CompareFunctionToDX(stencil_face_op.compare);

    return stencil_desc;
}

Ptr<RenderState> RenderState::Create(RenderContext& context, const RenderState::Settings& state_settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<RenderStateDX>(dynamic_cast<RenderContextBase&>(context), state_settings);
}

RenderStateDX::RenderStateDX(RenderContextBase& context, const Settings& settings)
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
    CD3DX12_RASTERIZER_DESC rasterizer_desc(D3D12_DEFAULT);
    rasterizer_desc.FillMode                    = ConvertRasterizerFillModeToD3D12(settings.rasterizer.fill_mode);
    rasterizer_desc.CullMode                    = ConvertRasterizerCullModeToD3D12(settings.rasterizer.cull_mode);
    rasterizer_desc.FrontCounterClockwise       = settings.rasterizer.is_front_counter_clockwise;
    rasterizer_desc.MultisampleEnable           = settings.rasterizer.sample_count > 1;
    rasterizer_desc.ForcedSampleCount           = !settings.depth.enabled && !settings.stencil.enabled ? settings.rasterizer.sample_count : 0;

    // Set Blending state descriptor
    CD3DX12_BLEND_DESC                          blend_desc(D3D12_DEFAULT);
    blend_desc.AlphaToCoverageEnable            = settings.rasterizer.alpha_to_coverage_enabled;
    blend_desc.IndependentBlendEnable           = settings.blending.is_independent;

    uint32_t rt_index = 0;
    for (const Blending::RenderTarget& render_target : settings.blending.render_targets)
    {
        // Set render target blending descriptor
        D3D12_RENDER_TARGET_BLEND_DESC& rt_blend_desc = blend_desc.RenderTarget[rt_index++];
        rt_blend_desc.BlendEnable               = render_target.blend_enabled;
        rt_blend_desc.RenderTargetWriteMask     = ConvertRenderTargetWriteMaskToD3D12(render_target.write_mask);
        rt_blend_desc.BlendOp                   = ConvertBlendingOperationToD3D12(render_target.rgb_blend_op);
        rt_blend_desc.BlendOpAlpha              = ConvertBlendingOperationToD3D12(render_target.alpha_blend_op);
        rt_blend_desc.SrcBlend                  = ConvertBlendingFactorToD3D12(render_target.source_rgb_blend_factor);
        rt_blend_desc.SrcBlendAlpha             = ConvertBlendingFactorToD3D12(render_target.source_alpha_blend_factor);
        rt_blend_desc.DestBlend                 = ConvertBlendingFactorToD3D12(render_target.dest_rgb_blend_factor);
        rt_blend_desc.DestBlendAlpha            = ConvertBlendingFactorToD3D12(render_target.dest_alpha_blend_factor);
    }

    // Set blending factor
    assert(settings.blending_color.size() <= 4);
    for (uint32_t component_index = 0; component_index < static_cast<uint32_t>(settings.blending_color.size()); ++component_index)
    {
        m_blend_factor[component_index] = settings.blending_color[component_index];
    }

    // Set depth and stencil state descriptor
    CD3DX12_DEPTH_STENCIL_DESC                  depth_stencil_desc(D3D12_DEFAULT);
    depth_stencil_desc.DepthEnable              = settings.depth.enabled;
    depth_stencil_desc.DepthWriteMask           = settings.depth.write_enabled ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    depth_stencil_desc.DepthFunc                = TypeConverterDX::CompareFunctionToDX(settings.depth.compare);
    depth_stencil_desc.StencilEnable            = settings.stencil.enabled;
    depth_stencil_desc.StencilReadMask          = settings.stencil.read_mask;
    depth_stencil_desc.StencilWriteMask         = settings.stencil.write_mask;
    depth_stencil_desc.FrontFace                = ConvertStencilFaceOperationsToD3D12(settings.stencil.front_face);
    depth_stencil_desc.BackFace                 = ConvertStencilFaceOperationsToD3D12(settings.stencil.back_face);

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
    m_pipeline_state_desc.SampleDesc.Count      = settings.rasterizer.sample_count;

    // Set RTV, DSV formats for pipeline state
    if (program_settings.color_formats.size() > g_max_rtv_count)
    {
        throw std::runtime_error("Number of color attachments (" + std::to_string(program_settings.color_formats.size()) +
                                 ") exceeds maximum RTV count in DirectX (" + std::to_string(g_max_rtv_count) + ")");
    }
    std::fill_n(m_pipeline_state_desc.RTVFormats, g_max_rtv_count, DXGI_FORMAT_UNKNOWN);
    uint32_t attachment_index = 0;
    for (PixelFormat color_format : program_settings.color_formats)
    {
        m_pipeline_state_desc.RTVFormats[attachment_index++] = TypeConverterDX::DataFormatToDXGI(color_format);
    }
    m_pipeline_state_desc.NumRenderTargets = static_cast<UINT>(program_settings.color_formats.size());
    m_pipeline_state_desc.DSVFormat = settings.depth.enabled ? TypeConverterDX::DataFormatToDXGI(program_settings.depth_format) : DXGI_FORMAT_UNKNOWN;

    m_viewports     = TypeConverterDX::ViewportsToD3D(settings.viewports);
    m_scissor_rects = TypeConverterDX::ScissorRectsToD3D(settings.scissor_rects);

    m_cp_pipeline_state.Reset();
}

void RenderStateDX::Apply(RenderCommandListBase& command_list, Group::Mask state_groups)
{
    ITT_FUNCTION_TASK();

    RenderCommandListDX& dx_render_command_list = static_cast<RenderCommandListDX&>(command_list);
    ID3D12GraphicsCommandList& d3d12_command_list = dx_render_command_list.GetNativeCommandList();

    if (state_groups & Group::Program    ||
        state_groups & Group::Rasterizer ||
        state_groups & Group::Blending   ||
        state_groups & Group::DepthStencil)
    {
        d3d12_command_list.SetPipelineState(GetNativePipelineState().Get());
    }

    d3d12_command_list.SetGraphicsRootSignature(GetProgramDX().GetNativeRootSignature().Get());

    if (state_groups & Group::Viewports)
    {
        d3d12_command_list.RSSetViewports(static_cast<UINT>(m_viewports.size()), m_viewports.data());
    }
    if (state_groups & Group::ScissorRects)
    {
        d3d12_command_list.RSSetScissorRects(static_cast<UINT>(m_scissor_rects.size()), m_scissor_rects.data());
    }
    if (state_groups & Group::BlendingColor)
    {
        d3d12_command_list.OMSetBlendFactor(m_blend_factor);
    }
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

void RenderStateDX::InitializeNativePipelineState()
{
    ITT_FUNCTION_TASK();
    if (m_cp_pipeline_state)
        return;

    ThrowIfFailed(GetRenderContextDX().GetDeviceDX().GetNativeDevice()->CreateGraphicsPipelineState(&m_pipeline_state_desc, IID_PPV_ARGS(&m_cp_pipeline_state)));
    SetName(GetName());
}

wrl::ComPtr<ID3D12PipelineState>& RenderStateDX::GetNativePipelineState()
{
    ITT_FUNCTION_TASK();
    if (!m_cp_pipeline_state)
    {
        InitializeNativePipelineState();
    }
    return m_cp_pipeline_state;
}

ProgramDX& RenderStateDX::GetProgramDX()
{
    ITT_FUNCTION_TASK();
    return static_cast<ProgramDX&>(GetProgram());
}

RenderContextDX& RenderStateDX::GetRenderContextDX()
{
    ITT_FUNCTION_TASK();
    return static_cast<RenderContextDX&>(GetRenderContext());
}

} // namespace Methane::Graphics
