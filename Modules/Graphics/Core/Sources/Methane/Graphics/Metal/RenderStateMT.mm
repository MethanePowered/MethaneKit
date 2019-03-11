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

FILE: Methane/Graphics/Metal/RenderStateMT.mm
Metal implementation of the render state interface.

******************************************************************************/

#include "RenderStateMT.h"
#include "ContextMT.h"
#include "RenderCommandListMT.h"
#include "ProgramMT.h"
#include "ShaderMT.h"
#include "TypesMT.h"

#include <Methane/Graphics/Instrumentation.h>

#import <Methane/Platform/MacOS/AppViewMT.h>
#import <Methane/Platform/MacOS/Types.h>

#include <cassert>

using namespace Methane::Graphics;

MTLCullMode ConvertRasterizerCullModeToMetal(RenderState::Rasterizer::CullMode cull_mode) noexcept
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


MTLTriangleFillMode ConvertRasterizerFillModeToMetal(RenderState::Rasterizer::FillMode fill_mode) noexcept
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

MTLStencilOperation ConvertStencilOperationToMetal(RenderState::Stencil::Operation operation) noexcept
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
        default:                                assert(0);
    }
    return MTLStencilOperationKeep;
}

MTLWinding ConvertRasterizerFrontWindingToMetal(bool is_front_counter_clockwise) noexcept
{
    ITT_FUNCTION_TASK();
    return is_front_counter_clockwise ? MTLWindingCounterClockwise : MTLWindingClockwise;
}

MTLStencilDescriptor* ConvertStencilDescriptorToMetal(const RenderState::Stencil& stencil, bool for_front_face)
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

RenderState::Ptr RenderState::Create(Context& context, const RenderState::Settings& state_settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<RenderStateMT>(static_cast<ContextBase&>(context), state_settings);
}

RenderStateMT::RenderStateMT(ContextBase& context, const Settings& settings)
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
    
    ProgramMT& metal_program = static_cast<ProgramMT&>(*m_settings.sp_program);

    // Program state
    m_mtl_pipeline_state_desc                           = [[MTLRenderPipelineDescriptor alloc] init];
    m_mtl_pipeline_state_desc.vertexFunction            = metal_program.GetNativeShaderFunction(Shader::Type::Vertex);
    m_mtl_pipeline_state_desc.fragmentFunction          = metal_program.GetNativeShaderFunction(Shader::Type::Pixel);
    m_mtl_pipeline_state_desc.vertexDescriptor          = metal_program.GetNativeVertexDescriptor();
    
    // Color, depth, stencil attachment formats state from program settings
    uint32_t attachment_index = 0;
    for (PixelFormat color_format : metal_program.GetSettings().color_formats)
    {
        m_mtl_pipeline_state_desc.colorAttachments[attachment_index++].pixelFormat = TypeConverterMT::DataFormatToMetalPixelType(color_format);
    }
    m_mtl_pipeline_state_desc.colorAttachments[attachment_index].pixelFormat = MTLPixelFormatInvalid;
    m_mtl_pipeline_state_desc.depthAttachmentPixelFormat   = TypeConverterMT::DataFormatToMetalPixelType(metal_program.GetSettings().depth_format);
    m_mtl_pipeline_state_desc.stencilAttachmentPixelFormat = MTLPixelFormatInvalid; // TODO: stencil not supported yet
    
    // Rasterizer state
    m_mtl_pipeline_state_desc.sampleCount               = m_settings.rasterizer.sample_count;
    m_mtl_pipeline_state_desc.alphaToCoverageEnabled    = m_settings.rasterizer.alpha_to_coverage_enabled ? YES : NO;
    m_mtl_pipeline_state_desc.alphaToOneEnabled         = m_settings.rasterizer.alpha_to_one_enabled      ? YES : NO;
    
    // Depth-stencil state
    m_mtl_depth_stencil_state_desc                      = [[MTLDepthStencilDescriptor alloc] init];
    m_mtl_depth_stencil_state_desc.depthWriteEnabled    = m_settings.depth.enabled ? YES : NO;
    m_mtl_depth_stencil_state_desc.depthCompareFunction = m_settings.depth.enabled ? TypeConverterMT::CompareFunctionToMetal(m_settings.depth.compare) : MTLCompareFunctionNever;
    m_mtl_depth_stencil_state_desc.backFaceStencil      = ConvertStencilDescriptorToMetal(m_settings.stencil, false);
    m_mtl_depth_stencil_state_desc.frontFaceStencil     = ConvertStencilDescriptorToMetal(m_settings.stencil, true);
    
    // Separate state parameters
    m_mtl_fill_mode          = ConvertRasterizerFillModeToMetal(m_settings.rasterizer.fill_mode);
    m_mtl_cull_mode          = ConvertRasterizerCullModeToMetal(m_settings.rasterizer.cull_mode);
    m_mtl_front_face_winding = ConvertRasterizerFrontWindingToMetal(m_settings.rasterizer.is_front_counter_clockwise);
    
    if (!m_settings.viewports.empty())
    {
        SetViewports(m_settings.viewports);
    }
    if (!m_settings.scissor_rects.empty())
    {
        SetScissorRects(m_settings.scissor_rects);
    }
    
    ResetNativeState();
}

void RenderStateMT::Apply(RenderCommandListBase& command_list)
{
    ITT_FUNCTION_TASK();

    RenderCommandListMT& metal_command_list = static_cast<RenderCommandListMT&>(command_list);
    id<MTLRenderCommandEncoder>& mtl_cmd_encoder = metal_command_list.GetNativeEncoder();
    
    [mtl_cmd_encoder setRenderPipelineState: GetNativePipelineState()];
    [mtl_cmd_encoder setDepthStencilState: GetNativeDepthState()];
    [mtl_cmd_encoder setTriangleFillMode: m_mtl_fill_mode];
    [mtl_cmd_encoder setFrontFacingWinding: m_mtl_front_face_winding];
    [mtl_cmd_encoder setCullMode: m_mtl_cull_mode];
    if (!m_mtl_viewports.empty())
    {
        [mtl_cmd_encoder setViewports: m_mtl_viewports.data() count:static_cast<uint32_t>(m_mtl_viewports.size())];
    }
    if (!m_mtl_scissor_rects.empty())
    {
        [mtl_cmd_encoder setScissorRects: m_mtl_scissor_rects.data() count:static_cast<uint32_t>(m_mtl_scissor_rects.size())];
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
        mtl_viewport.originX = viewport.origin.x();
        mtl_viewport.originY = viewport.origin.y();
        mtl_viewport.width   = viewport.size.width;
        mtl_viewport.height  = viewport.size.height;
        mtl_viewport.znear   = viewport.origin.z();
        mtl_viewport.zfar    = viewport.origin.z() + viewport.size.depth;
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
        mtl_scissor_rect.x      = static_cast<NSUInteger>(scissor_rect.origin.x());
        mtl_scissor_rect.y      = static_cast<NSUInteger>(scissor_rect.origin.y());
        mtl_scissor_rect.width  = static_cast<NSUInteger>(scissor_rect.size.width);
        mtl_scissor_rect.height = static_cast<NSUInteger>(scissor_rect.size.height);
        m_mtl_scissor_rects.emplace_back(std::move(mtl_scissor_rect));
    }
}

void RenderStateMT::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    RenderStateBase::SetName(name);
    
    NSString* ns_name = Methane::MacOS::ConvertToNSType<std::string, NSString*>(name);
    m_mtl_pipeline_state_desc.label      = ns_name;
    m_mtl_depth_stencil_state_desc.label = ns_name;
    
    ResetNativeState();
}

id<MTLRenderPipelineState>& RenderStateMT::GetNativePipelineState()
{
    ITT_FUNCTION_TASK();

    if (!m_mtl_pipeline_state)
    {
        NSError* ns_error = nil;
        m_mtl_pipeline_state = [GetContextMT().GetNativeDevice() newRenderPipelineStateWithDescriptor:m_mtl_pipeline_state_desc error:&ns_error];
        if (!m_mtl_pipeline_state)
        {
            const std::string error_msg = MacOS::ConvertFromNSType<NSString, std::string>([ns_error localizedDescription]);
            throw std::runtime_error("Failed to create Metal pipeline state: " + error_msg);
        }
    }
    return m_mtl_pipeline_state;
}

id<MTLDepthStencilState>& RenderStateMT::GetNativeDepthState()
{
    ITT_FUNCTION_TASK();

    if (!m_mtl_depth_state)
    {
        m_mtl_depth_state = [GetContextMT().GetNativeDevice() newDepthStencilStateWithDescriptor:m_mtl_depth_stencil_state_desc];
        if (!m_mtl_depth_state)
        {
            throw std::runtime_error("Failed to create Metal depth state.");
        }
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

ContextMT& RenderStateMT::GetContextMT() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<class ContextMT&>(m_context);
}
