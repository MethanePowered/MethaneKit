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

FILE: Methane/Graphics/DirectX/RenderState.cpp
DirectX 12 implementation of the render state interface.

******************************************************************************/

#include <Methane/Graphics/DirectX/RenderState.h>
#include <Methane/Graphics/DirectX/RenderContext.h>
#include <Methane/Graphics/DirectX/Device.h>
#include <Methane/Graphics/DirectX/Program.h>
#include <Methane/Graphics/DirectX/Shader.h>
#include <Methane/Graphics/DirectX/Types.h>
#include <Methane/Graphics/DirectX/Texture.h>
#include <Methane/Graphics/DirectX/RenderCommandList.h>
#include <Methane/Graphics/DirectX/ErrorHandling.h>

#include <Methane/Graphics/RHI/IRenderPass.h>

#include <Methane/Platform/Windows/Utils.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <nowide/convert.hpp>
#include <directx/d3dx12_core.h>
#include <d3dcompiler.h>

#include <algorithm>

namespace Methane::Graphics::DirectX
{

constexpr size_t g_max_rtv_count = sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC::RTVFormats) / sizeof(DXGI_FORMAT);

[[nodiscard]]
inline CD3DX12_SHADER_BYTECODE GetShaderByteCode(const Ptr<Rhi::IShader>& shader_ptr)
{
    META_FUNCTION_TASK();
    const Data::Chunk* byte_code_chunk_ptr = shader_ptr ? static_cast<const Shader&>(*shader_ptr).GetNativeByteCode() : nullptr;
    return byte_code_chunk_ptr
        ? CD3DX12_SHADER_BYTECODE(byte_code_chunk_ptr->GetDataPtr(), byte_code_chunk_ptr->GetDataSize())
        : CD3DX12_SHADER_BYTECODE(nullptr, 0);
}

[[nodiscard]]
static D3D12_FILL_MODE ConvertRasterizerFillModeToD3D12(Rhi::IRenderState::Rasterizer::FillMode fill_mode)
{
    META_FUNCTION_TASK();
    using RasterizerFillMode = Rhi::IRenderState::Rasterizer::FillMode;
    
    switch (fill_mode)
    {
    case RasterizerFillMode::Solid:     return D3D12_FILL_MODE_SOLID;
    case RasterizerFillMode::Wireframe: return D3D12_FILL_MODE_WIREFRAME;
    default:                            META_UNEXPECTED_RETURN(fill_mode, D3D12_FILL_MODE_SOLID);
    }
}

[[nodiscard]]
static D3D12_CULL_MODE ConvertRasterizerCullModeToD3D12(Rhi::IRenderState::Rasterizer::CullMode cull_mode)
{
    META_FUNCTION_TASK();
    using RasterizerCullMode = Rhi::IRenderState::Rasterizer::CullMode;

    switch (cull_mode)
    {
    case RasterizerCullMode::None:      return D3D12_CULL_MODE_NONE;
    case RasterizerCullMode::Front:     return D3D12_CULL_MODE_FRONT;
    case RasterizerCullMode::Back:      return D3D12_CULL_MODE_BACK;
    default:                            META_UNEXPECTED_RETURN(cull_mode, D3D12_CULL_MODE_NONE);
    }
}

[[nodiscard]]
static UINT8 ConvertRenderTargetColorWriteMaskToD3D12(Rhi::BlendingColorChannelMask rt_color_write)
{
    META_FUNCTION_TASK();
    UINT8 d3d12_color_write_mask = 0;

    if (rt_color_write.HasAnyBit(Rhi::BlendingColorChannel::Red))
        d3d12_color_write_mask |= D3D12_COLOR_WRITE_ENABLE_RED;   // NOSONAR

    if (rt_color_write.HasAnyBit(Rhi::BlendingColorChannel::Green))
        d3d12_color_write_mask |= D3D12_COLOR_WRITE_ENABLE_GREEN; // NOSONAR

    if (rt_color_write.HasAnyBit(Rhi::BlendingColorChannel::Blue))
        d3d12_color_write_mask |= D3D12_COLOR_WRITE_ENABLE_BLUE;  // NOSONAR

    if (rt_color_write.HasAnyBit(Rhi::BlendingColorChannel::Alpha))
        d3d12_color_write_mask |= D3D12_COLOR_WRITE_ENABLE_ALPHA; // NOSONAR

    return d3d12_color_write_mask;
};

[[nodiscard]]
static D3D12_BLEND_OP ConvertBlendingOperationToD3D12(Rhi::IRenderState::Blending::Operation blend_operation)
{
    META_FUNCTION_TASK();
    using BlendOp = Rhi::IRenderState::Blending::Operation;

    switch(blend_operation)
    {
    case BlendOp::Add:              return D3D12_BLEND_OP_ADD;
    case BlendOp::Subtract:         return D3D12_BLEND_OP_SUBTRACT;
    case BlendOp::ReverseSubtract:  return D3D12_BLEND_OP_REV_SUBTRACT;
    case BlendOp::Minimum:          return D3D12_BLEND_OP_MIN;
    case BlendOp::Maximum:          return D3D12_BLEND_OP_MAX;
    default:                        META_UNEXPECTED_RETURN(blend_operation, D3D12_BLEND_OP_ADD);
    }
}

[[nodiscard]]
static D3D12_BLEND ConvertBlendingFactorToD3D12(Rhi::IRenderState::Blending::Factor blend_factor)
{
    META_FUNCTION_TASK();
    using BlendFactor = Rhi::IRenderState::Blending::Factor;
    
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
    default:                                    META_UNEXPECTED_RETURN(blend_factor, D3D12_BLEND_ZERO);
    }
}

[[nodiscard]]
static D3D12_STENCIL_OP ConvertStencilOperationToD3D12(Rhi::FaceOperation operation)
{
    META_FUNCTION_TASK();

    switch (operation)
    {
    case Rhi::FaceOperation::Keep:            return D3D12_STENCIL_OP_KEEP;
    case Rhi::FaceOperation::Zero:            return D3D12_STENCIL_OP_ZERO;
    case Rhi::FaceOperation::Replace:         return D3D12_STENCIL_OP_REPLACE;
    case Rhi::FaceOperation::Invert:          return D3D12_STENCIL_OP_INVERT;
    case Rhi::FaceOperation::IncrementClamp:  return D3D12_STENCIL_OP_INCR_SAT;
    case Rhi::FaceOperation::DecrementClamp:  return D3D12_STENCIL_OP_DECR_SAT;
    case Rhi::FaceOperation::IncrementWrap:   return D3D12_STENCIL_OP_INCR;
    case Rhi::FaceOperation::DecrementWrap:   return D3D12_STENCIL_OP_DECR;
    default:                             META_UNEXPECTED_RETURN(operation, D3D12_STENCIL_OP_KEEP);
    }
}

[[nodiscard]]
static D3D12_DEPTH_STENCILOP_DESC ConvertStencilFaceOperationsToD3D12(const Rhi::FaceOperations& stencil_face_op)
{
    META_FUNCTION_TASK();
    D3D12_DEPTH_STENCILOP_DESC stencil_desc{};

    stencil_desc.StencilFailOp      = ConvertStencilOperationToD3D12(stencil_face_op.stencil_failure);
    stencil_desc.StencilPassOp      = ConvertStencilOperationToD3D12(stencil_face_op.stencil_pass);
    stencil_desc.StencilDepthFailOp = ConvertStencilOperationToD3D12(stencil_face_op.depth_failure);
    stencil_desc.StencilFunc        = TypeConverter::CompareFunctionToD3D(stencil_face_op.compare);

    return stencil_desc;
}

RenderState::RenderState(const Base::RenderContext& context, const Settings& settings)
    : Base::RenderState(context, settings)
{
    META_FUNCTION_TASK();
    Reset(settings); // NOSONAR - method is not overridable in final class
}

void RenderState::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    Base::RenderState::Reset(settings);

    // Set Rasterizer state descriptor
    CD3DX12_RASTERIZER_DESC rasterizer_desc(D3D12_DEFAULT);
    rasterizer_desc.FillMode              = ConvertRasterizerFillModeToD3D12(settings.rasterizer.fill_mode);
    rasterizer_desc.CullMode              = ConvertRasterizerCullModeToD3D12(settings.rasterizer.cull_mode);
    rasterizer_desc.FrontCounterClockwise = settings.rasterizer.is_front_counter_clockwise;
    rasterizer_desc.MultisampleEnable     = settings.rasterizer.sample_count > 1;
    rasterizer_desc.ForcedSampleCount     = !settings.depth.enabled && !settings.stencil.enabled ? settings.rasterizer.sample_count : 0;

    // Set Blending state descriptor
    CD3DX12_BLEND_DESC blend_desc(D3D12_DEFAULT);
    blend_desc.AlphaToCoverageEnable  = settings.rasterizer.alpha_to_coverage_enabled;
    blend_desc.IndependentBlendEnable = settings.blending.is_independent;

    uint32_t rt_index = 0;
    for (const Blending::RenderTarget& render_target : settings.blending.render_targets)
    {
        // Set render target blending descriptor
        D3D12_RENDER_TARGET_BLEND_DESC& rt_blend_desc = blend_desc.RenderTarget[rt_index++];
        rt_blend_desc.BlendEnable           = render_target.blend_enabled;
        rt_blend_desc.RenderTargetWriteMask = ConvertRenderTargetColorWriteMaskToD3D12(render_target.color_write);
        rt_blend_desc.BlendOp               = ConvertBlendingOperationToD3D12(render_target.rgb_blend_op);
        rt_blend_desc.BlendOpAlpha          = ConvertBlendingOperationToD3D12(render_target.alpha_blend_op);
        rt_blend_desc.SrcBlend              = ConvertBlendingFactorToD3D12(render_target.source_rgb_blend_factor);
        rt_blend_desc.SrcBlendAlpha         = ConvertBlendingFactorToD3D12(render_target.source_alpha_blend_factor);
        rt_blend_desc.DestBlend             = ConvertBlendingFactorToD3D12(render_target.dest_rgb_blend_factor);
        rt_blend_desc.DestBlendAlpha        = ConvertBlendingFactorToD3D12(render_target.dest_alpha_blend_factor);
    }

    // Set blending factor
    META_CHECK_LESS(settings.blending_color.GetSize(), 5);
    for (Data::Size component_index = 0; component_index < settings.blending_color.GetSize(); ++component_index)
    {
        m_blend_factor[component_index] = settings.blending_color[component_index];
    }

    // Set depth and stencil state descriptor
    CD3DX12_DEPTH_STENCIL_DESC depth_stencil_desc(D3D12_DEFAULT);
    depth_stencil_desc.DepthEnable      = settings.depth.enabled;
    depth_stencil_desc.DepthWriteMask   = settings.depth.write_enabled ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    depth_stencil_desc.DepthFunc        = TypeConverter::CompareFunctionToD3D(settings.depth.compare);
    depth_stencil_desc.StencilEnable    = settings.stencil.enabled;
    depth_stencil_desc.StencilReadMask  = settings.stencil.read_mask;
    depth_stencil_desc.StencilWriteMask = settings.stencil.write_mask;
    depth_stencil_desc.FrontFace        = ConvertStencilFaceOperationsToD3D12(settings.stencil.front_face);
    depth_stencil_desc.BackFace         = ConvertStencilFaceOperationsToD3D12(settings.stencil.back_face);

    // Set pipeline state descriptor for program
    const Program& dx_program                 = RenderState::GetDirectProgram();
    m_pipeline_state_desc.InputLayout           = dx_program.GetNativeInputLayoutDesc();
    m_pipeline_state_desc.pRootSignature        = dx_program.GetNativeRootSignature().Get();
    m_pipeline_state_desc.VS                    = GetShaderByteCode(dx_program.GetShader(Rhi::ShaderType::Vertex));
    m_pipeline_state_desc.PS                    = GetShaderByteCode(dx_program.GetShader(Rhi::ShaderType::Pixel));
    m_pipeline_state_desc.DepthStencilState     = depth_stencil_desc;
    m_pipeline_state_desc.BlendState            = blend_desc;
    m_pipeline_state_desc.RasterizerState       = rasterizer_desc;
    m_pipeline_state_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // Not used: for GS or HS shaders only
    m_pipeline_state_desc.SampleMask            = UINT_MAX;
    m_pipeline_state_desc.SampleDesc.Count      = settings.rasterizer.sample_count;

    // Set RTV, DSV formats for pipeline state
    const AttachmentFormats attachment_formats = settings.render_pattern_ptr->GetAttachmentFormats();
    META_CHECK_LESS_DESCR(attachment_formats.colors.size(), g_max_rtv_count + 1,
                              "number of color attachments exceeds maximum RTV count in DirectX");
    std::fill_n(m_pipeline_state_desc.RTVFormats, g_max_rtv_count, DXGI_FORMAT_UNKNOWN);
    uint32_t attachment_index = 0;
    for (PixelFormat color_format : attachment_formats.colors)
    {
        m_pipeline_state_desc.RTVFormats[attachment_index++] = TypeConverter::PixelFormatToDxgi(color_format);
    }
    m_pipeline_state_desc.NumRenderTargets = static_cast<UINT>(attachment_formats.colors.size());
    m_pipeline_state_desc.DSVFormat = settings.depth.enabled
                                    ? TypeConverter::PixelFormatToDxgi(attachment_formats.depth)
                                    : DXGI_FORMAT_UNKNOWN;

    m_pipeline_state_cptr.Reset();
}

void RenderState::Apply(Base::RenderCommandList& command_list, Groups state_groups)
{
    META_FUNCTION_TASK();
    const auto& dx_render_command_list = static_cast<RenderCommandList&>(command_list);
    ID3D12GraphicsCommandList& d3d12_command_list = dx_render_command_list.GetNativeCommandList();

    if (state_groups.HasAnyBits({Group::Program, Group::Rasterizer, Group::Blending, Group::DepthStencil}))
    {
        d3d12_command_list.SetPipelineState(GetNativePipelineState().Get());
    }

    d3d12_command_list.SetGraphicsRootSignature(GetDirectProgram().GetNativeRootSignature().Get());

    if (state_groups.HasAnyBit(Group::BlendingColor))
    {
        d3d12_command_list.OMSetBlendFactor(m_blend_factor.data());
    }
}

bool RenderState::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Base::RenderState::SetName(name))
        return false;

    if (m_pipeline_state_cptr)
    {
        m_pipeline_state_cptr->SetName(nowide::widen(name).c_str());
    }
    return true;
}

void RenderState::InitializeNativePipelineState()
{
    META_FUNCTION_TASK();
    if (m_pipeline_state_cptr)
        return;

    const wrl::ComPtr<ID3D12Device>& native_device_cptr = GetDirectRenderContext().GetDirectDevice().GetNativeDevice();
    ThrowIfFailed(native_device_cptr->CreateGraphicsPipelineState(&m_pipeline_state_desc, IID_PPV_ARGS(&m_pipeline_state_cptr)), native_device_cptr.Get());
    SetName(GetName());
}

wrl::ComPtr<ID3D12PipelineState>& RenderState::GetNativePipelineState()
{
    META_FUNCTION_TASK();
    if (!m_pipeline_state_cptr)
    {
        InitializeNativePipelineState();
    }
    return m_pipeline_state_cptr;
}

Program& RenderState::GetDirectProgram()
{
    META_FUNCTION_TASK();
    return static_cast<Program&>(GetProgram());
}

const RenderContext& RenderState::GetDirectRenderContext() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const RenderContext&>(GetRenderContext());
}

} // namespace Methane::Graphics::DirectX
