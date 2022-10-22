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

FILE: Methane/Graphics/Metal/RenderStateMT.mm
Metal implementation of the render state interface.

******************************************************************************/

#include "RenderStateMT.hh"
#include "RenderContextMT.hh"
#include "RenderCommandListMT.hh"
#include "ProgramMT.hh"
#include "ShaderMT.hh"
#include "TypesMT.hh"

#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>

namespace Methane::Graphics
{

static MTLCullMode ConvertRasterizerCullModeToMetal(RenderState::Rasterizer::CullMode cull_mode) noexcept
{
    META_FUNCTION_TASK();
    using RasterizerCullMode = RenderState::Rasterizer::CullMode;

    switch(cull_mode)
    {
        case RasterizerCullMode::None:  return MTLCullModeNone;
        case RasterizerCullMode::Back:  return MTLCullModeBack;
        case RasterizerCullMode::Front: return MTLCullModeFront;
        default:                        META_UNEXPECTED_ARG_RETURN(cull_mode, MTLCullModeNone);
    }
}

static MTLTriangleFillMode ConvertRasterizerFillModeToMetal(RenderState::Rasterizer::FillMode fill_mode) noexcept
{
    META_FUNCTION_TASK();
    using RasterizerFillMode = RenderState::Rasterizer::FillMode;

    switch(fill_mode)
    {
        case RasterizerFillMode::Solid:     return MTLTriangleFillModeFill;
        case RasterizerFillMode::Wireframe: return MTLTriangleFillModeLines;
        default:                            META_UNEXPECTED_ARG_RETURN(fill_mode, MTLTriangleFillModeFill);
    }
}
    
static MTLColorWriteMask ConvertRenderTargetWriteMaskToMetal(RenderState::Blending::ColorChannels rt_write_mask)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;
    using ColorChannels = RenderState::Blending::ColorChannels;

    MTLColorWriteMask mtl_color_write_mask = 0U;
    if (static_cast<bool>(rt_write_mask & ColorChannels::Red))
        mtl_color_write_mask |= MTLColorWriteMaskRed;
    if (static_cast<bool>(rt_write_mask & ColorChannels::Green))
        mtl_color_write_mask |= MTLColorWriteMaskGreen;
    if (static_cast<bool>(rt_write_mask & ColorChannels::Blue))
        mtl_color_write_mask |= MTLColorWriteMaskBlue;
    if (static_cast<bool>(rt_write_mask & ColorChannels::Alpha))
        mtl_color_write_mask |= MTLColorWriteMaskAlpha;
    return mtl_color_write_mask;
};

static MTLBlendOperation ConvertBlendingOperationToMetal(RenderState::Blending::Operation blend_operation)
{
    META_FUNCTION_TASK();
    using BlendOp = RenderState::Blending::Operation;

    switch(blend_operation)
    {
    case BlendOp::Add:              return MTLBlendOperationAdd;
    case BlendOp::Subtract:         return MTLBlendOperationSubtract;
    case BlendOp::ReverseSubtract:  return MTLBlendOperationReverseSubtract;
    case BlendOp::Minimum:          return MTLBlendOperationMin;
    case BlendOp::Maximum:          return MTLBlendOperationMax;
    default:                        META_UNEXPECTED_ARG_RETURN(blend_operation, MTLBlendOperationAdd);
    }
}

static MTLBlendFactor ConvertBlendingFactorToMetal(RenderState::Blending::Factor blend_factor)
{
    META_FUNCTION_TASK();
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
    default:                                    META_UNEXPECTED_ARG_RETURN(blend_factor, MTLBlendFactorZero);
    }
}

static MTLStencilOperation ConvertStencilOperationToMetal(RenderState::Stencil::Operation operation) noexcept
{
    META_FUNCTION_TASK();
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
        default:                                META_UNEXPECTED_ARG_RETURN(operation, MTLStencilOperationKeep);
    }
}

static MTLWinding ConvertRasterizerFrontWindingToMetal(bool is_front_counter_clockwise) noexcept
{
    META_FUNCTION_TASK();
    return is_front_counter_clockwise ? MTLWindingCounterClockwise : MTLWindingClockwise;
}

static MTLStencilDescriptor* ConvertStencilDescriptorToMetal(const RenderState::Stencil& stencil, bool for_front_face)
{
    META_FUNCTION_TASK();
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

static std::vector<MTLViewport> ConvertViewportsToMetal(const Viewports& viewports)
{
    META_FUNCTION_TASK();
    std::vector<MTLViewport> mtl_viewports;
    mtl_viewports.reserve(viewports.size());

    for(const Viewport& viewport : viewports)
    {
        MTLViewport mtl_viewport{ };
        mtl_viewport.originX = viewport.origin.GetX();
        mtl_viewport.originY = viewport.origin.GetY();
        mtl_viewport.width   = viewport.size.GetWidth();
        mtl_viewport.height  = viewport.size.GetHeight();
        mtl_viewport.znear   = viewport.origin.GetZ();
        mtl_viewport.zfar    = viewport.origin.GetZ() + viewport.size.GetDepth();
        mtl_viewports.emplace_back(std::move(mtl_viewport));
    }

    return mtl_viewports;
}

static std::vector<MTLScissorRect> ConvertScissorRectsToMetal(const ScissorRects& scissor_rects)
{
    META_FUNCTION_TASK();
    std::vector<MTLScissorRect> mtl_scissor_rects;
    mtl_scissor_rects.reserve(scissor_rects.size());

    for(const ScissorRect& scissor_rect : scissor_rects)
    {
        MTLScissorRect mtl_scissor_rect{};
        mtl_scissor_rect.x      = static_cast<NSUInteger>(scissor_rect.origin.GetX());
        mtl_scissor_rect.y      = static_cast<NSUInteger>(scissor_rect.origin.GetY());
        mtl_scissor_rect.width  = static_cast<NSUInteger>(scissor_rect.size.GetWidth());
        mtl_scissor_rect.height = static_cast<NSUInteger>(scissor_rect.size.GetHeight());
        mtl_scissor_rects.emplace_back(std::move(mtl_scissor_rect));
    }

    return mtl_scissor_rects;
}

Ptr<ViewState> ViewState::Create(const ViewState::Settings& state_settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<ViewStateMT>(state_settings);
}

ViewStateMT::ViewStateMT(const Settings& settings)
    : ViewStateBase(settings)
    , m_mtl_viewports(ConvertViewportsToMetal(settings.viewports))
    , m_mtl_scissor_rects(ConvertScissorRectsToMetal(settings.scissor_rects))
{
    META_FUNCTION_TASK();
}

bool ViewStateMT::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    if (!ViewStateBase::Reset(settings))
        return false;

    m_mtl_viewports     = ConvertViewportsToMetal(settings.viewports);
    m_mtl_scissor_rects = ConvertScissorRectsToMetal(settings.scissor_rects);
    return true;
}

bool ViewStateMT::SetViewports(const Viewports& viewports)
{
    META_FUNCTION_TASK();
    if (!ViewStateBase::SetViewports(viewports))
        return false;

    m_mtl_viewports = ConvertViewportsToMetal(viewports);
    return true;
}

bool ViewStateMT::SetScissorRects(const ScissorRects& scissor_rects)
{
    META_FUNCTION_TASK();
    if (!ViewStateBase::SetScissorRects(scissor_rects))
        return false;

    m_mtl_scissor_rects = ConvertScissorRectsToMetal(scissor_rects);
    return true;
}

void ViewStateMT::Apply(RenderCommandListBase& command_list)
{
    META_FUNCTION_TASK();

    RenderCommandListMT& metal_command_list = static_cast<RenderCommandListMT&>(command_list);
    const id<MTLRenderCommandEncoder>& mtl_cmd_encoder = metal_command_list.GetNativeCommandEncoder();

    [mtl_cmd_encoder setViewports: m_mtl_viewports.data() count:static_cast<uint32_t>(m_mtl_viewports.size())];
    [mtl_cmd_encoder setScissorRects: m_mtl_scissor_rects.data() count:static_cast<uint32_t>(m_mtl_scissor_rects.size())];
}

Ptr<RenderState> RenderState::Create(const IRenderContext& context, const RenderState::Settings& state_settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderStateMT>(dynamic_cast<const RenderContextBase&>(context), state_settings);
}

RenderStateMT::RenderStateMT(const RenderContextBase& context, const Settings& settings)
    : RenderStateBase(context, settings)
{
    META_FUNCTION_TASK();
    Reset(settings);
}

void RenderStateMT::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    RenderStateBase::Reset(settings);

    ProgramMT& metal_program = static_cast<ProgramMT&>(*settings.program_ptr);

    // IProgram state
    m_mtl_pipeline_state_desc                           = [[MTLRenderPipelineDescriptor alloc] init];
    m_mtl_pipeline_state_desc.vertexFunction            = metal_program.GetNativeShaderFunction(ShaderType::Vertex);
    m_mtl_pipeline_state_desc.fragmentFunction          = metal_program.GetNativeShaderFunction(ShaderType::Pixel);
    m_mtl_pipeline_state_desc.vertexDescriptor          = metal_program.GetNativeVertexDescriptor();
    
    // Rasterizer state
    m_mtl_pipeline_state_desc.sampleCount               = settings.rasterizer.sample_count;
    m_mtl_pipeline_state_desc.alphaToCoverageEnabled    = settings.rasterizer.alpha_to_coverage_enabled;
    m_mtl_pipeline_state_desc.alphaToOneEnabled         = NO; // not supported by Methane
    
    // Blending state
    const AttachmentFormats attach_formats = settings.render_pattern_ptr->GetAttachmentFormats();
    for (uint32_t rt_index = 0; rt_index < settings.blending.render_targets.size(); ++rt_index)
    {
        const Blending::RenderTarget& render_target     = settings.blending.is_independent
                                                        ? settings.blending.render_targets[rt_index]
                                                        : settings.blending.render_targets[0];
        
        // Set render target blending state for color attachment
        MTLRenderPipelineColorAttachmentDescriptor* mtl_color_attach = m_mtl_pipeline_state_desc.colorAttachments[rt_index];
        mtl_color_attach.pixelFormat                    = rt_index < attach_formats.colors.size()
                                                        ? TypeConverterMT::DataFormatToMetalPixelType(attach_formats.colors[rt_index])
                                                        : MTLPixelFormatInvalid;
        mtl_color_attach.blendingEnabled                = render_target.blend_enabled && rt_index < attach_formats.colors.size();
        mtl_color_attach.writeMask                      = ConvertRenderTargetWriteMaskToMetal(render_target.write_mask);
        mtl_color_attach.rgbBlendOperation              = ConvertBlendingOperationToMetal(render_target.rgb_blend_op);
        mtl_color_attach.alphaBlendOperation            = ConvertBlendingOperationToMetal(render_target.alpha_blend_op);
        mtl_color_attach.sourceRGBBlendFactor           = ConvertBlendingFactorToMetal(render_target.source_rgb_blend_factor);
        mtl_color_attach.sourceAlphaBlendFactor         = ConvertBlendingFactorToMetal(render_target.source_alpha_blend_factor);
        mtl_color_attach.destinationRGBBlendFactor      = ConvertBlendingFactorToMetal(render_target.dest_rgb_blend_factor);
        mtl_color_attach.destinationAlphaBlendFactor    = ConvertBlendingFactorToMetal(render_target.dest_alpha_blend_factor);
    }
    
    // Color, depth, stencil attachment formats state from program settings
    m_mtl_pipeline_state_desc.depthAttachmentPixelFormat   = TypeConverterMT::DataFormatToMetalPixelType(attach_formats.depth);
    m_mtl_pipeline_state_desc.stencilAttachmentPixelFormat = TypeConverterMT::DataFormatToMetalPixelType(attach_formats.stencil);
    
    // Depth-stencil state
    m_mtl_depth_stencil_state_desc                      = [[MTLDepthStencilDescriptor alloc] init];
    m_mtl_depth_stencil_state_desc.depthWriteEnabled    = settings.depth.write_enabled && attach_formats.depth != PixelFormat::Unknown;
    m_mtl_depth_stencil_state_desc.depthCompareFunction = settings.depth.enabled && attach_formats.depth != PixelFormat::Unknown
                                                        ? TypeConverterMT::CompareFunctionToMetal(settings.depth.compare)
                                                        : MTLCompareFunctionAlways;
    m_mtl_depth_stencil_state_desc.backFaceStencil      = ConvertStencilDescriptorToMetal(settings.stencil, false);
    m_mtl_depth_stencil_state_desc.frontFaceStencil     = ConvertStencilDescriptorToMetal(settings.stencil, true);
    
    // Separate state parameters
    m_mtl_fill_mode          = ConvertRasterizerFillModeToMetal(settings.rasterizer.fill_mode);
    m_mtl_cull_mode          = ConvertRasterizerCullModeToMetal(settings.rasterizer.cull_mode);
    m_mtl_front_face_winding = ConvertRasterizerFrontWindingToMetal(settings.rasterizer.is_front_counter_clockwise);
    
    ResetNativeState();
}

void RenderStateMT::Apply(RenderCommandListBase& command_list, Groups state_groups)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    RenderCommandListMT& metal_command_list = static_cast<RenderCommandListMT&>(command_list);
    const id<MTLRenderCommandEncoder>& mtl_cmd_encoder = metal_command_list.GetNativeCommandEncoder();
    
    if (static_cast<bool>(state_groups & Groups::Program)    ||
        static_cast<bool>(state_groups & Groups::Rasterizer) ||
        static_cast<bool>(state_groups & Groups::Blending))
    {
        [mtl_cmd_encoder setRenderPipelineState: GetNativePipelineState()];
    }
    if (static_cast<bool>(state_groups & Groups::DepthStencil))
    {
        [mtl_cmd_encoder setDepthStencilState: GetNativeDepthStencilState()];
    }
    if (static_cast<bool>(state_groups & Groups::Rasterizer))
    {
        [mtl_cmd_encoder setTriangleFillMode: m_mtl_fill_mode];
        [mtl_cmd_encoder setFrontFacingWinding: m_mtl_front_face_winding];
        [mtl_cmd_encoder setCullMode: m_mtl_cull_mode];
    }
    if (static_cast<bool>(state_groups & Groups::BlendingColor))
    {
        const Settings& settings = GetSettings();
        [mtl_cmd_encoder setBlendColorRed:settings.blending_color.GetRed()
                                    green:settings.blending_color.GetGreen()
                                     blue:settings.blending_color.GetBlue()
                                    alpha:settings.blending_color.GetAlpha()];
    }
}

bool RenderStateMT::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!RenderStateBase::SetName(name))
        return false;
    
    NSString* ns_name = Methane::MacOS::ConvertToNsType<std::string, NSString*>(name);
    m_mtl_pipeline_state_desc.label      = ns_name;
    m_mtl_depth_stencil_state_desc.label = ns_name;
    
    ResetNativeState();
    return true;
}
    
void RenderStateMT::InitializeNativeStates()
{
    META_FUNCTION_TASK();
    InitializeNativePipelineState();
    InitializeNativeDepthStencilState();
}

void RenderStateMT::InitializeNativePipelineState()
{
    META_FUNCTION_TASK();
    if (m_mtl_pipeline_state)
        return;
    
    NSError* ns_error = nil;
    m_mtl_pipeline_state = [GetRenderContextMT().GetDeviceMT().GetNativeDevice() newRenderPipelineStateWithDescriptor:m_mtl_pipeline_state_desc error:&ns_error];
    META_CHECK_ARG_NOT_NULL_DESCR(m_mtl_pipeline_state,
                                  "failed to create Metal pipeline state: {}",
                                  MacOS::ConvertFromNsType<NSString, std::string>([ns_error localizedDescription]));
}

void RenderStateMT::InitializeNativeDepthStencilState()
{
    META_FUNCTION_TASK();
    if (m_mtl_depth_state)
        return;

    META_CHECK_ARG_NOT_NULL(m_mtl_depth_stencil_state_desc);
    m_mtl_depth_state = [GetRenderContextMT().GetDeviceMT().GetNativeDevice() newDepthStencilStateWithDescriptor:m_mtl_depth_stencil_state_desc];
    META_CHECK_ARG_NOT_NULL_DESCR(m_mtl_depth_state, "failed to create Metal depth state");
}

id<MTLRenderPipelineState> RenderStateMT::GetNativePipelineState()
{
    META_FUNCTION_TASK();
    if (!m_mtl_pipeline_state)
    {
        InitializeNativePipelineState();
    }
    return m_mtl_pipeline_state;
}

id<MTLDepthStencilState> RenderStateMT::GetNativeDepthStencilState()
{
    META_FUNCTION_TASK();
    if (!m_mtl_depth_state)
    {
        InitializeNativeStates();
    }
    return m_mtl_depth_state;
}

void RenderStateMT::ResetNativeState()
{
    META_FUNCTION_TASK();
    m_mtl_pipeline_state = nil;
    m_mtl_depth_state = nil;
}

const RenderContextMT& RenderStateMT::GetRenderContextMT() const
{
    META_FUNCTION_TASK();
    return dynamic_cast<const RenderContextMT&>(GetRenderContext());
}

} // namespace Methane::Graphics
