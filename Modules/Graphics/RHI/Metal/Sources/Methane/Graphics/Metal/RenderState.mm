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

FILE: Methane/Graphics/Metal/RenderState.mm
Metal implementation of the render state interface.

******************************************************************************/

#include <Methane/Graphics/Metal/RenderState.hh>
#include <Methane/Graphics/Metal/RenderContext.hh>
#include <Methane/Graphics/Metal/RenderCommandList.hh>
#include <Methane/Graphics/Metal/Program.hh>
#include <Methane/Graphics/Metal/Shader.hh>
#include <Methane/Graphics/Metal/Types.hh>

#include <Methane/Graphics/RHI/IRenderPass.h>

#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Metal
{

static MTLCullMode ConvertRasterizerCullModeToMetal(Rhi::RasterizerCullMode cull_mode) noexcept
{
    META_FUNCTION_TASK();
    switch(cull_mode)
    {
        using enum Rhi::RasterizerCullMode;
        case None:  return MTLCullModeNone;
        case Back:  return MTLCullModeBack;
        case Front: return MTLCullModeFront;
        default: META_UNEXPECTED_RETURN(cull_mode, MTLCullModeNone);
    }
}

static MTLTriangleFillMode ConvertRasterizerFillModeToMetal(Rhi::RasterizerFillMode fill_mode) noexcept
{
    META_FUNCTION_TASK();
    switch(fill_mode)
    {
        using enum Rhi::RasterizerFillMode;
        case Solid:     return MTLTriangleFillModeFill;
        case Wireframe: return MTLTriangleFillModeLines;
        default: META_UNEXPECTED_RETURN(fill_mode, MTLTriangleFillModeFill);
    }
}
    
static MTLColorWriteMask ConvertRenderTargetColorWriteMaskToMetal(Rhi::BlendingColorChannelMask color_channels)
{
    META_FUNCTION_TASK();
    MTLColorWriteMask mtl_color_write_mask = 0U;

    using enum Rhi::BlendingColorChannel;
    if (color_channels.HasAnyBit(Red))
        mtl_color_write_mask |= MTLColorWriteMaskRed;

    if (color_channels.HasAnyBit(Green))
        mtl_color_write_mask |= MTLColorWriteMaskGreen;

    if (color_channels.HasAnyBit(Blue))
        mtl_color_write_mask |= MTLColorWriteMaskBlue;

    if (color_channels.HasAnyBit(Alpha))
        mtl_color_write_mask |= MTLColorWriteMaskAlpha;

    return mtl_color_write_mask;
};

static MTLBlendOperation ConvertBlendingOperationToMetal(Rhi::BlendingOperation blend_operation)
{
    META_FUNCTION_TASK();
    switch(blend_operation)
    {
    using enum Rhi::BlendingOperation;
    case Add:              return MTLBlendOperationAdd;
    case Subtract:         return MTLBlendOperationSubtract;
    case ReverseSubtract:  return MTLBlendOperationReverseSubtract;
    case Minimum:          return MTLBlendOperationMin;
    case Maximum:          return MTLBlendOperationMax;
    default: META_UNEXPECTED_RETURN(blend_operation, MTLBlendOperationAdd);
    }
}

static MTLBlendFactor ConvertBlendingFactorToMetal(Rhi::BlendingFactor blend_factor)
{
    META_FUNCTION_TASK();
    switch (blend_factor)
    {
    using enum Rhi::BlendingFactor;
    case Zero:                     return MTLBlendFactorZero;
    case One:                      return MTLBlendFactorOne;
    case SourceColor:              return MTLBlendFactorSourceColor;
    case OneMinusSourceColor:      return MTLBlendFactorOneMinusSourceColor;
    case SourceAlpha:              return MTLBlendFactorSourceAlpha;
    case OneMinusSourceAlpha:      return MTLBlendFactorOneMinusSourceAlpha;
    case DestinationColor:         return MTLBlendFactorDestinationColor;
    case OneMinusDestinationColor: return MTLBlendFactorOneMinusDestinationColor;
    case DestinationAlpha:         return MTLBlendFactorDestinationAlpha;
    case OneMinusDestinationAlpha: return MTLBlendFactorOneMinusDestinationAlpha;
    case SourceAlphaSaturated:     return MTLBlendFactorSourceAlphaSaturated;
    case BlendColor:               return MTLBlendFactorBlendColor;
    case OneMinusBlendColor:       return MTLBlendFactorOneMinusBlendColor;
    case BlendAlpha:               return MTLBlendFactorBlendAlpha;
    case OneMinusBlendAlpha:       return MTLBlendFactorOneMinusBlendAlpha;
    case Source1Color:             return MTLBlendFactorSource1Color;
    case OneMinusSource1Color:     return MTLBlendFactorOneMinusSource1Color;
    case Source1Alpha:             return MTLBlendFactorSource1Alpha;
    case OneMinusSource1Alpha:     return MTLBlendFactorOneMinusSource1Alpha;
    default: META_UNEXPECTED_RETURN(blend_factor, MTLBlendFactorZero);
    }
}

static MTLStencilOperation ConvertStencilOperationToMetal(Rhi::FaceOperation operation) noexcept
{
    META_FUNCTION_TASK();
    switch(operation)
    {
        using enum Rhi::FaceOperation;
        case Keep:            return MTLStencilOperationKeep;
        case Zero:            return MTLStencilOperationZero;
        case Replace:         return MTLStencilOperationReplace;
        case Invert:          return MTLStencilOperationInvert;
        case IncrementClamp:  return MTLStencilOperationIncrementClamp;
        case DecrementClamp:  return MTLStencilOperationDecrementClamp;
        case IncrementWrap:   return MTLStencilOperationIncrementWrap;
        case DecrementWrap:   return MTLStencilOperationDecrementWrap;
        default: META_UNEXPECTED_RETURN(operation, MTLStencilOperationKeep);
    }
}

static MTLWinding ConvertRasterizerFrontWindingToMetal(bool is_front_counter_clockwise) noexcept
{
    META_FUNCTION_TASK();
    return is_front_counter_clockwise ? MTLWindingCounterClockwise : MTLWindingClockwise;
}

static MTLStencilDescriptor* ConvertStencilDescriptorToMetal(const Rhi::IRenderState::Stencil& stencil, bool for_front_face)
{
    META_FUNCTION_TASK();
    if (!stencil.enabled)
        return nil;
    
    const Rhi::FaceOperations& face_operations = for_front_face ? stencil.front_face : stencil.back_face;
    
    MTLStencilDescriptor* mtl_stencil_desc      = [[MTLStencilDescriptor alloc] init];
    mtl_stencil_desc.stencilFailureOperation    = ConvertStencilOperationToMetal(face_operations.stencil_failure);
    mtl_stencil_desc.depthFailureOperation      = ConvertStencilOperationToMetal(face_operations.depth_failure);
    mtl_stencil_desc.depthStencilPassOperation  = ConvertStencilOperationToMetal(face_operations.depth_stencil_pass);
    mtl_stencil_desc.stencilCompareFunction     = TypeConverter::CompareFunctionToMetal(face_operations.compare);
    mtl_stencil_desc.readMask                   = stencil.read_mask;
    mtl_stencil_desc.writeMask                  = stencil.write_mask;
    
    return mtl_stencil_desc;
}

RenderState::RenderState(const Base::RenderContext& context, const Settings& settings)
    : Base::RenderState(context, settings)
{
    META_FUNCTION_TASK();
    Reset(settings);
}

void RenderState::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    Base::RenderState::Reset(settings);

    Program& metal_program = static_cast<Program&>(*settings.program_ptr);

    // IProgram state
    m_mtl_pipeline_state_desc                  = [[MTLRenderPipelineDescriptor alloc] init];
    m_mtl_pipeline_state_desc.vertexFunction   = metal_program.GetNativeShaderFunction(Rhi::ShaderType::Vertex);
    m_mtl_pipeline_state_desc.fragmentFunction = metal_program.GetNativeShaderFunction(Rhi::ShaderType::Pixel);
    m_mtl_pipeline_state_desc.vertexDescriptor = metal_program.GetNativeVertexDescriptor();
    
    // Rasterizer state
    m_mtl_pipeline_state_desc.rasterSampleCount      = settings.rasterizer.sample_count;
    m_mtl_pipeline_state_desc.alphaToCoverageEnabled = settings.rasterizer.alpha_to_coverage_enabled;
    m_mtl_pipeline_state_desc.alphaToOneEnabled      = NO; // not supported by Methane
    
    // Blending state
    const AttachmentFormats attach_formats = settings.render_pattern_ptr->GetAttachmentFormats();
    for (uint32_t rt_index = 0; rt_index < settings.blending.render_targets.size(); ++rt_index)
    {
        const Blending::RenderTarget& render_target = settings.blending.is_independent
                                                    ? settings.blending.render_targets[rt_index]
                                                    : settings.blending.render_targets[0];
        
        // Set render target blending state for color attachment
        MTLRenderPipelineColorAttachmentDescriptor* mtl_color_attach = m_mtl_pipeline_state_desc.colorAttachments[rt_index];
        mtl_color_attach.pixelFormat                 = rt_index < attach_formats.colors.size()
                                                     ? TypeConverter::DataFormatToMetalPixelType(attach_formats.colors[rt_index])
                                                     : MTLPixelFormatInvalid;
        mtl_color_attach.blendingEnabled             = render_target.blend_enabled && rt_index < attach_formats.colors.size();
        mtl_color_attach.writeMask                   = ConvertRenderTargetColorWriteMaskToMetal(render_target.color_write);
        mtl_color_attach.rgbBlendOperation           = ConvertBlendingOperationToMetal(render_target.rgb_blend_op);
        mtl_color_attach.alphaBlendOperation         = ConvertBlendingOperationToMetal(render_target.alpha_blend_op);
        mtl_color_attach.sourceRGBBlendFactor        = ConvertBlendingFactorToMetal(render_target.source_rgb_blend_factor);
        mtl_color_attach.sourceAlphaBlendFactor      = ConvertBlendingFactorToMetal(render_target.source_alpha_blend_factor);
        mtl_color_attach.destinationRGBBlendFactor   = ConvertBlendingFactorToMetal(render_target.dest_rgb_blend_factor);
        mtl_color_attach.destinationAlphaBlendFactor = ConvertBlendingFactorToMetal(render_target.dest_alpha_blend_factor);
    }
    
    // Color, depth, stencil attachment formats state from program settings
    m_mtl_pipeline_state_desc.depthAttachmentPixelFormat   = TypeConverter::DataFormatToMetalPixelType(attach_formats.depth);
    m_mtl_pipeline_state_desc.stencilAttachmentPixelFormat = TypeConverter::DataFormatToMetalPixelType(attach_formats.stencil);
    
    // Depth-stencil state
    m_mtl_depth_stencil_state_desc                      = [[MTLDepthStencilDescriptor alloc] init];
    m_mtl_depth_stencil_state_desc.depthWriteEnabled    = settings.depth.write_enabled && attach_formats.depth != PixelFormat::Unknown;
    m_mtl_depth_stencil_state_desc.depthCompareFunction = settings.depth.enabled && attach_formats.depth != PixelFormat::Unknown
                                                        ? TypeConverter::CompareFunctionToMetal(settings.depth.compare)
                                                        : MTLCompareFunctionAlways;
    m_mtl_depth_stencil_state_desc.backFaceStencil      = ConvertStencilDescriptorToMetal(settings.stencil, false);
    m_mtl_depth_stencil_state_desc.frontFaceStencil     = ConvertStencilDescriptorToMetal(settings.stencil, true);
    
    // Separate state parameters
    m_mtl_fill_mode          = ConvertRasterizerFillModeToMetal(settings.rasterizer.fill_mode);
    m_mtl_cull_mode          = ConvertRasterizerCullModeToMetal(settings.rasterizer.cull_mode);
    m_mtl_front_face_winding = ConvertRasterizerFrontWindingToMetal(settings.rasterizer.is_front_counter_clockwise);
    
    ResetNativeState();
}

void RenderState::Apply(Base::RenderCommandList& command_list, Groups state_groups)
{
    META_FUNCTION_TASK();
    RenderCommandList& metal_command_list = static_cast<RenderCommandList&>(command_list);
    const id<MTLRenderCommandEncoder>& mtl_cmd_encoder = metal_command_list.GetNativeCommandEncoder();
    
    if (state_groups.HasAnyBits({ Group::Program, Group::Rasterizer, Group::Blending }))
    {
        [mtl_cmd_encoder setRenderPipelineState: GetNativePipelineState()];
    }
    if (state_groups.HasAnyBit(Group::DepthStencil))
    {
        [mtl_cmd_encoder setDepthStencilState: GetNativeDepthStencilState()];
    }
    if (state_groups.HasAnyBit(Group::Rasterizer))
    {
        [mtl_cmd_encoder setTriangleFillMode: m_mtl_fill_mode];
        [mtl_cmd_encoder setFrontFacingWinding: m_mtl_front_face_winding];
        [mtl_cmd_encoder setCullMode: m_mtl_cull_mode];
    }
    if (state_groups.HasAnyBit(Group::BlendingColor))
    {
        const Settings& settings = GetSettings();
        [mtl_cmd_encoder setBlendColorRed:settings.blending_color.GetRed()
                                    green:settings.blending_color.GetGreen()
                                     blue:settings.blending_color.GetBlue()
                                    alpha:settings.blending_color.GetAlpha()];
    }
}

bool RenderState::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Base::RenderState::SetName(name))
        return false;
    
    NSString* ns_name = MacOS::ConvertToNsString(name);
    m_mtl_pipeline_state_desc.label      = ns_name;
    m_mtl_depth_stencil_state_desc.label = ns_name;
    
    ResetNativeState();
    return true;
}
    
void RenderState::InitializeNativeStates()
{
    META_FUNCTION_TASK();
    InitializeNativePipelineState();
    InitializeNativeDepthStencilState();
}

void RenderState::InitializeNativePipelineState()
{
    META_FUNCTION_TASK();
    if (m_mtl_pipeline_state)
        return;
    
    NSError* ns_error = nil;
    m_mtl_pipeline_state = [GetMetalRenderContext().GetMetalDevice().GetNativeDevice() newRenderPipelineStateWithDescriptor:m_mtl_pipeline_state_desc error:&ns_error];
    META_CHECK_NOT_NULL_DESCR(m_mtl_pipeline_state,
                              "failed to create Metal render pipeline state: {}",
                              MacOS::ConvertFromNsString([ns_error localizedDescription]));
}

void RenderState::InitializeNativeDepthStencilState()
{
    META_FUNCTION_TASK();
    if (m_mtl_depth_state)
        return;

    META_CHECK_NOT_NULL(m_mtl_depth_stencil_state_desc);
    m_mtl_depth_state = [GetMetalRenderContext().GetMetalDevice().GetNativeDevice() newDepthStencilStateWithDescriptor:m_mtl_depth_stencil_state_desc];
    META_CHECK_NOT_NULL_DESCR(m_mtl_depth_state, "failed to create Metal depth-stencil state");
}

id<MTLRenderPipelineState> RenderState::GetNativePipelineState()
{
    META_FUNCTION_TASK();
    if (!m_mtl_pipeline_state)
    {
        InitializeNativePipelineState();
    }
    return m_mtl_pipeline_state;
}

id<MTLDepthStencilState> RenderState::GetNativeDepthStencilState()
{
    META_FUNCTION_TASK();
    if (!m_mtl_depth_state)
    {
        InitializeNativeStates();
    }
    return m_mtl_depth_state;
}

void RenderState::ResetNativeState()
{
    META_FUNCTION_TASK();
    m_mtl_pipeline_state = nil;
    m_mtl_depth_state = nil;
}

const RenderContext& RenderState::GetMetalRenderContext() const
{
    META_FUNCTION_TASK();
    return dynamic_cast<const RenderContext&>(GetRenderContext());
}

} // namespace Methane::Graphics::Metal
