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

FILE: Methane/Graphics/Metal/RenderStateMT.mm
Metal implementation of the render state interface.

******************************************************************************/

#include "RenderStateMT.hh"
#include "RenderContextMT.hh"
#include "RenderCommandListMT.hh"
#include "ProgramMT.hh"
#include "ShaderMT.hh"
#include "TypesMT.hh"

#include <Methane/Platform/MacOS/Types.hh>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

static MTLCullMode ConvertRasterizerCullModeToMetal(RenderState::Rasterizer::CullMode cull_mode) noexcept
{
    ITT_FUNCTION_TASK();

    using RasterizerCullMode = RenderState::Rasterizer::CullMode;

    switch(cull_mode)
    {
        case RasterizerCullMode::None:  return MTLCullModeNone;
        case RasterizerCullMode::Back:  return MTLCullModeBack;
        case RasterizerCullMode::Front: return MTLCullModeFront;
    }
    return MTLCullModeNone;
}

static MTLTriangleFillMode ConvertRasterizerFillModeToMetal(RenderState::Rasterizer::FillMode fill_mode) noexcept
{
    ITT_FUNCTION_TASK();

    using RasterizerFillMode = RenderState::Rasterizer::FillMode;

    switch(fill_mode)
    {
        case RasterizerFillMode::Solid:     return MTLTriangleFillModeFill;
        case RasterizerFillMode::Wireframe: return MTLTriangleFillModeLines;
    }
    return MTLTriangleFillModeFill;
}
    
static MTLColorWriteMask ConvertRenderTargetWriteMaskToMetal(RenderState::Blending::ColorChannel::Mask rt_write_mask)
{
    ITT_FUNCTION_TASK();

    using ColorChannel = RenderState::Blending::ColorChannel;

    MTLColorWriteMask mtl_color_write_mask = 0u;
    if (rt_write_mask & ColorChannel::Red)
        mtl_color_write_mask |= MTLColorWriteMaskRed;
    if (rt_write_mask & ColorChannel::Green)
        mtl_color_write_mask |= MTLColorWriteMaskGreen;
    if (rt_write_mask & ColorChannel::Blue)
        mtl_color_write_mask |= MTLColorWriteMaskBlue;
    if (rt_write_mask & ColorChannel::Alpha)
        mtl_color_write_mask |= MTLColorWriteMaskAlpha;
    return mtl_color_write_mask;
};

static MTLBlendOperation ConvertBlendingOperationToMetal(RenderState::Blending::Operation blend_operation)
{
    ITT_FUNCTION_TASK();

    using BlendOp = RenderState::Blending::Operation;

    switch(blend_operation)
    {
    case BlendOp::Add:              return MTLBlendOperationAdd;
    case BlendOp::Subtract:         return MTLBlendOperationSubtract;
    case BlendOp::ReverseSubtract:  return MTLBlendOperationReverseSubtract;
    case BlendOp::Minimum:          return MTLBlendOperationMin;
    case BlendOp::Maximum:          return MTLBlendOperationMax;
    }
    return MTLBlendOperationAdd;
}

static MTLBlendFactor ConvertBlendingFactorToMetal(RenderState::Blending::Factor blend_factor)
{
    ITT_FUNCTION_TASK();

    using BlendFactor = RenderState::Blending::Factor;
    
    switch (blend_factor)
    {
    case BlendFactor::Zero:                     return MTLBlendFactorZero;
    case BlendFactor::One:                      return MTLBlendFactorOne;
    case BlendFactor::SourceColor:              return MTLBlendFactorSourceColor;
    case BlendFactor::OneMinusSourceColor:      return MTLBlendFactorOneMinusSourceColor;
    case BlendFactor::SourceAlpha:              return MTLBlendFactorSourceAlpha;
    case BlendFactor::OneMinusSourceAlpha:      return MTLBlendFactorOneMinusSourceAlpha;
    case BlendFactor::DestinationColor:         return MTLBlendFactorDestinationColor;
    case BlendFactor::OneMinusDestinationColor: return MTLBlendFactorOneMinusDestinationColor;
    case BlendFactor::DestinationAlpha:         return MTLBlendFactorDestinationAlpha;
    case BlendFactor::OneMinusDestinationAlpha: return MTLBlendFactorOneMinusDestinationAlpha;
    case BlendFactor::SourceAlphaSaturated:     return MTLBlendFactorSourceAlphaSaturated;
    case BlendFactor::BlendColor:               return MTLBlendFactorBlendColor;
    case BlendFactor::OneMinusBlendColor:       return MTLBlendFactorOneMinusBlendColor;
    case BlendFactor::BlendAlpha:               return MTLBlendFactorBlendAlpha;
    case BlendFactor::OneMinusBlendAlpha:       return MTLBlendFactorOneMinusBlendAlpha;
    case BlendFactor::Source1Color:             return MTLBlendFactorSource1Color;
    case BlendFactor::OneMinusSource1Color:     return MTLBlendFactorOneMinusSource1Color;
    case BlendFactor::Source1Alpha:             return MTLBlendFactorSource1Alpha;
    case BlendFactor::OneMinusSource1Alpha:     return MTLBlendFactorOneMinusSource1Alpha;
    }
    return MTLBlendFactorZero;
}

static MTLStencilOperation ConvertStencilOperationToMetal(RenderState::Stencil::Operation operation) noexcept
{
    ITT_FUNCTION_TASK();

    using StencilOperation = RenderState::Stencil::Operation;

    switch(operation)
    {
        case StencilOperation::Keep:            return MTLStencilOperationKeep;
        case StencilOperation::Zero:            return MTLStencilOperationZero;
        case StencilOperation::Replace:         return MTLStencilOperationReplace;
        case StencilOperation::Invert:          return MTLStencilOperationInvert;
        case StencilOperation::IncrementClamp:  return MTLStencilOperationIncrementClamp;
        case StencilOperation::DecrementClamp:  return MTLStencilOperationDecrementClamp;
        case StencilOperation::IncrementWrap:   return MTLStencilOperationIncrementWrap;
        case StencilOperation::DecrementWrap:   return MTLStencilOperationDecrementWrap;
    }
    return MTLStencilOperationKeep;
}

static MTLWinding ConvertRasterizerFrontWindingToMetal(bool is_front_counter_clockwise) noexcept
{
    ITT_FUNCTION_TASK();
    return is_front_counter_clockwise ? MTLWindingCounterClockwise : MTLWindingClockwise;
}

static MTLStencilDescriptor* ConvertStencilDescriptorToMetal(const RenderState::Stencil& stencil, bool for_front_face)
{
    ITT_FUNCTION_TASK();
    if (!stencil.enabled)
        return nil;
    
    const RenderState::Stencil::FaceOperations& face_operations = for_front_face ? stencil.front_face : stencil.back_face;
    
    MTLStencilDescriptor* mtl_stencil_desc      = [[MTLStencilDescriptor alloc] init];
    mtl_stencil_desc.stencilFailureOperation    = ConvertStencilOperationToMetal(face_operations.stencil_failure);
    mtl_stencil_desc.depthFailureOperation      = ConvertStencilOperationToMetal(face_operations.depth_failure);
    mtl_stencil_desc.depthStencilPassOperation  = ConvertStencilOperationToMetal(face_operations.depth_stencil_pass);
    mtl_stencil_desc.stencilCompareFunction     = TypeConverterMT::CompareFunctionToMetal(face_operations.compare);
    mtl_stencil_desc.readMask                   = stencil.read_mask;
    mtl_stencil_desc.writeMask                  = stencil.write_mask;
    
    return mtl_stencil_desc;
}

Ptr<RenderState> RenderState::Create(RenderContext& context, const RenderState::Settings& state_settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<RenderStateMT>(dynamic_cast<RenderContextBase&>(context), state_settings);
}

RenderStateMT::RenderStateMT(RenderContextBase& context, const Settings& settings)
    : RenderStateBase(context, settings)
{
    ITT_FUNCTION_TASK();
    Reset(settings);
}

RenderStateMT::~RenderStateMT()
{
    ITT_FUNCTION_TASK();

    [m_mtl_pipeline_state_desc release];
    [m_mtl_depth_stencil_state_desc release];
    [m_mtl_pipeline_state release];
    [m_mtl_depth_state release];
}

void RenderStateMT::Reset(const Settings& settings)
{
    ITT_FUNCTION_TASK();
    if (!settings.sp_program)
    {
        throw std::invalid_argument("Can not create state with empty program.");
    }
    
    RenderStateBase::Reset(settings);
    [m_mtl_pipeline_state_desc release];
    [m_mtl_depth_stencil_state_desc release];

    ProgramMT& metal_program = static_cast<ProgramMT&>(*settings.sp_program);

    // Program state
    m_mtl_pipeline_state_desc                           = [[MTLRenderPipelineDescriptor alloc] init];
    m_mtl_pipeline_state_desc.vertexFunction            = metal_program.GetNativeShaderFunction(Shader::Type::Vertex);
    m_mtl_pipeline_state_desc.fragmentFunction          = metal_program.GetNativeShaderFunction(Shader::Type::Pixel);
    m_mtl_pipeline_state_desc.vertexDescriptor          = metal_program.GetNativeVertexDescriptor();
    
    // Rasterizer state
    m_mtl_pipeline_state_desc.sampleCount               = settings.rasterizer.sample_count;
    m_mtl_pipeline_state_desc.alphaToCoverageEnabled    = settings.rasterizer.alpha_to_coverage_enabled;
    m_mtl_pipeline_state_desc.alphaToOneEnabled         = NO; // not supported by Methane
    
    // Blending state
    const std::vector<PixelFormat>& rt_color_formats = metal_program.GetSettings().color_formats;
    for (uint32_t rt_index = 0; rt_index < settings.blending.render_targets.size(); ++rt_index)
    {
        const Blending::RenderTarget& render_target     = settings.blending.is_independent
                                                        ? settings.blending.render_targets[rt_index]
                                                        : settings.blending.render_targets[0];
        
        // Set render target blending state for color attachment
        MTLRenderPipelineColorAttachmentDescriptor* mtl_color_attach = m_mtl_pipeline_state_desc.colorAttachments[rt_index];
        mtl_color_attach.pixelFormat                    = rt_index < rt_color_formats.size()
                                                        ? TypeConverterMT::DataFormatToMetalPixelType(rt_color_formats[rt_index])
                                                        : MTLPixelFormatInvalid;
        mtl_color_attach.blendingEnabled                = render_target.blend_enabled && rt_index < rt_color_formats.size();
        mtl_color_attach.writeMask                      = ConvertRenderTargetWriteMaskToMetal(render_target.write_mask);
        mtl_color_attach.rgbBlendOperation              = ConvertBlendingOperationToMetal(render_target.rgb_blend_op);
        mtl_color_attach.alphaBlendOperation            = ConvertBlendingOperationToMetal(render_target.alpha_blend_op);
        mtl_color_attach.sourceRGBBlendFactor           = ConvertBlendingFactorToMetal(render_target.source_rgb_blend_factor);
        mtl_color_attach.sourceAlphaBlendFactor         = ConvertBlendingFactorToMetal(render_target.source_alpha_blend_factor);
        mtl_color_attach.destinationRGBBlendFactor      = ConvertBlendingFactorToMetal(render_target.dest_rgb_blend_factor);
        mtl_color_attach.destinationAlphaBlendFactor    = ConvertBlendingFactorToMetal(render_target.dest_alpha_blend_factor);
    }
    
    // Color, depth, stencil attachment formats state from program settings
    const PixelFormat depth_format = metal_program.GetSettings().depth_format;
    m_mtl_pipeline_state_desc.depthAttachmentPixelFormat   = TypeConverterMT::DataFormatToMetalPixelType(depth_format);
    m_mtl_pipeline_state_desc.stencilAttachmentPixelFormat = MTLPixelFormatInvalid; // TODO: stencil not supported yet
    
    // Depth-stencil state
    m_mtl_depth_stencil_state_desc                      = [[MTLDepthStencilDescriptor alloc] init];
    m_mtl_depth_stencil_state_desc.depthWriteEnabled    = settings.depth.write_enabled && depth_format != PixelFormat::Unknown;
    m_mtl_depth_stencil_state_desc.depthCompareFunction = settings.depth.enabled && depth_format != PixelFormat::Unknown
                                                        ? TypeConverterMT::CompareFunctionToMetal(settings.depth.compare)
                                                        : MTLCompareFunctionAlways;
    m_mtl_depth_stencil_state_desc.backFaceStencil      = ConvertStencilDescriptorToMetal(settings.stencil, false);
    m_mtl_depth_stencil_state_desc.frontFaceStencil     = ConvertStencilDescriptorToMetal(settings.stencil, true);
    
    // Separate state parameters
    m_mtl_fill_mode          = ConvertRasterizerFillModeToMetal(settings.rasterizer.fill_mode);
    m_mtl_cull_mode          = ConvertRasterizerCullModeToMetal(settings.rasterizer.cull_mode);
    m_mtl_front_face_winding = ConvertRasterizerFrontWindingToMetal(settings.rasterizer.is_front_counter_clockwise);
    
    if (!settings.viewports.empty())
    {
        SetViewports(settings.viewports);
    }
    if (!settings.scissor_rects.empty())
    {
        SetScissorRects(settings.scissor_rects);
    }
    
    ResetNativeState();
}

void RenderStateMT::Apply(RenderCommandListBase& command_list, Group::Mask state_groups)
{
    ITT_FUNCTION_TASK();

    RenderCommandListMT& metal_command_list = static_cast<RenderCommandListMT&>(command_list);
    id<MTLRenderCommandEncoder>& mtl_cmd_encoder = metal_command_list.GetNativeRenderEncoder();
    
    if (state_groups & Group::Program    ||
        state_groups & Group::Rasterizer ||
        state_groups & Group::Blending)
    {
        [mtl_cmd_encoder setRenderPipelineState: GetNativePipelineState()];
    }
    if (state_groups & Group::DepthStencil)
    {
        [mtl_cmd_encoder setDepthStencilState: GetNativeDepthStencilState()];
    }
    if (state_groups & Group::Rasterizer)
    {
        [mtl_cmd_encoder setTriangleFillMode: m_mtl_fill_mode];
        [mtl_cmd_encoder setFrontFacingWinding: m_mtl_front_face_winding];
        [mtl_cmd_encoder setCullMode: m_mtl_cull_mode];
    }
    if (state_groups & Group::Viewports && !m_mtl_viewports.empty())
    {
        [mtl_cmd_encoder setViewports: m_mtl_viewports.data() count:static_cast<uint32_t>(m_mtl_viewports.size())];
    }
    if (state_groups & Group::ScissorRects && !m_mtl_scissor_rects.empty())
    {
        [mtl_cmd_encoder setScissorRects: m_mtl_scissor_rects.data() count:static_cast<uint32_t>(m_mtl_scissor_rects.size())];
    }
    if (state_groups & Group::BlendingColor)
    {
        const Settings& settings = GetSettings();
        [mtl_cmd_encoder setBlendColorRed:settings.blending_color.GetR()
                                    green:settings.blending_color.GetG()
                                     blue:settings.blending_color.GetB()
                                    alpha:settings.blending_color.GetA()];
    }
}

void RenderStateMT::SetViewports(const Viewports& viewports)
{
    ITT_FUNCTION_TASK();

    RenderStateBase::SetViewports(viewports);
    
    m_mtl_viewports.clear();
    for(const Viewport& viewport : viewports)
    {
        MTLViewport mtl_viewport = { };
        mtl_viewport.originX = viewport.origin.GetX();
        mtl_viewport.originY = viewport.origin.GetY();
        mtl_viewport.width   = viewport.size.width;
        mtl_viewport.height  = viewport.size.height;
        mtl_viewport.znear   = viewport.origin.GetZ();
        mtl_viewport.zfar    = viewport.origin.GetZ() + viewport.size.depth;
        m_mtl_viewports.emplace_back(std::move(mtl_viewport));
    }
}

void RenderStateMT::SetScissorRects(const ScissorRects& scissor_rects)
{
    ITT_FUNCTION_TASK();

    RenderStateBase::SetScissorRects(scissor_rects);
    
    m_mtl_scissor_rects.clear();
    for(const ScissorRect& scissor_rect : scissor_rects)
    {
        MTLScissorRect mtl_scissor_rect = {};
        mtl_scissor_rect.x      = static_cast<NSUInteger>(scissor_rect.origin.GetX());
        mtl_scissor_rect.y      = static_cast<NSUInteger>(scissor_rect.origin.GetY());
        mtl_scissor_rect.width  = static_cast<NSUInteger>(scissor_rect.size.width);
        mtl_scissor_rect.height = static_cast<NSUInteger>(scissor_rect.size.height);
        m_mtl_scissor_rects.emplace_back(std::move(mtl_scissor_rect));
    }
}

void RenderStateMT::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    RenderStateBase::SetName(name);
    
    NSString* ns_name = Methane::MacOS::ConvertToNsType<std::string, NSString*>(name);
    m_mtl_pipeline_state_desc.label      = ns_name;
    m_mtl_depth_stencil_state_desc.label = ns_name;
    
    ResetNativeState();
}
    
void RenderStateMT::InitializeNativeStates()
{
    ITT_FUNCTION_TASK();
    InitializeNativePipelineState();
    InitializeNativeDepthStencilState();
}

void RenderStateMT::InitializeNativePipelineState()
{
    ITT_FUNCTION_TASK();
    if (m_mtl_pipeline_state)
        return;
    
    NSError* ns_error = nil;
    m_mtl_pipeline_state = [GetRenderContextMT().GetDeviceMT().GetNativeDevice() newRenderPipelineStateWithDescriptor:m_mtl_pipeline_state_desc error:&ns_error];
    if (!m_mtl_pipeline_state)
    {
        const std::string error_msg = MacOS::ConvertFromNsType<NSString, std::string>([ns_error localizedDescription]);
        throw std::runtime_error("Failed to create Metal pipeline state: " + error_msg);
    }
}

void RenderStateMT::InitializeNativeDepthStencilState()
{
    ITT_FUNCTION_TASK();
    if (m_mtl_depth_state)
        return;
    
    assert(m_mtl_depth_stencil_state_desc != nil);
    m_mtl_depth_state = [GetRenderContextMT().GetDeviceMT().GetNativeDevice() newDepthStencilStateWithDescriptor:m_mtl_depth_stencil_state_desc];
    if (!m_mtl_depth_state)
    {
        throw std::runtime_error("Failed to create Metal depth state.");
    }
}

id<MTLRenderPipelineState>& RenderStateMT::GetNativePipelineState()
{
    ITT_FUNCTION_TASK();
    if (!m_mtl_pipeline_state)
    {
        InitializeNativePipelineState();
    }
    return m_mtl_pipeline_state;
}

id<MTLDepthStencilState>& RenderStateMT::GetNativeDepthStencilState()
{
    ITT_FUNCTION_TASK();
    if (!m_mtl_depth_state)
    {
        InitializeNativeStates();
    }
    return m_mtl_depth_state;
}

void RenderStateMT::ResetNativeState()
{
    ITT_FUNCTION_TASK();

    [m_mtl_pipeline_state release];
    m_mtl_pipeline_state = nil;
    
    [m_mtl_depth_state release];
    m_mtl_depth_state = nil;
}

RenderContextMT& RenderStateMT::GetRenderContextMT()
{
    ITT_FUNCTION_TASK();
    return dynamic_cast<RenderContextMT&>(GetRenderContext());
}

} // namespace Methane::Graphics
