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

FILE: Methane/Graphics/Vulkan/RenderStateVK.cpp
Vulkan implementation of the render state interface.

******************************************************************************/

#include "RenderStateVK.h"
#include "RenderPassVK.h"
#include "ContextVK.h"
#include "DeviceVK.h"
#include "RenderCommandListVK.h"
#include "ProgramVK.h"
#include "ShaderVK.h"
#include "TypesVK.h"
#include "UtilsVK.hpp"

#include <Methane/Graphics/RenderContextBase.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>
#include <algorithm>

namespace Methane::Graphics
{

[[nodiscard]]
static vk::PolygonMode RasterizerFillModeToVulkan(RenderState::Rasterizer::FillMode fill_mode)
{
    META_FUNCTION_TASK();
    using FillMode = RenderState::Rasterizer::FillMode;
    switch(fill_mode)
    {
    case FillMode::Solid:     return vk::PolygonMode::eFill;
    case FillMode::Wireframe: return vk::PolygonMode::eLine;
    default:
        META_UNEXPECTED_ARG_RETURN(fill_mode, vk::PolygonMode::eFill);
    }
}

[[nodiscard]]
static vk::CullModeFlags RasterizerCullModeToVulkan(RenderState::Rasterizer::CullMode cull_mode)
{
    META_FUNCTION_TASK();
    using CullMode = RenderState::Rasterizer::CullMode;
    switch(cull_mode)
    {
    case CullMode::None:  return vk::CullModeFlagBits::eNone;
    case CullMode::Back:  return vk::CullModeFlagBits::eBack;
    case CullMode::Front: return vk::CullModeFlagBits::eFront;
    default:
        META_UNEXPECTED_ARG_RETURN(cull_mode, vk::CullModeFlagBits::eNone);
    }
}

[[nodiscard]]
static vk::SampleCountFlagBits RasterizerSampleCountToVulkan(uint32_t sample_count)
{
    META_FUNCTION_TASK();
    switch(sample_count)
    {
    case 1:  return vk::SampleCountFlagBits::e1;
    case 2:  return vk::SampleCountFlagBits::e2;
    case 4:  return vk::SampleCountFlagBits::e4;
    case 8:  return vk::SampleCountFlagBits::e8;
    case 16: return vk::SampleCountFlagBits::e16;
    case 32: return vk::SampleCountFlagBits::e32;
    case 61: return vk::SampleCountFlagBits::e64;
    default:
        META_UNEXPECTED_ARG_DESCR_RETURN(sample_count, vk::SampleCountFlagBits::e1, "Vulkan rasterizer sample count should be a power of 2 from 1 to 64.");
    }
}

[[nodiscard]]
static vk::StencilOp StencilOperationToVulkan(RenderState::Stencil::Operation stencil_operation)
{
    META_FUNCTION_TASK();
    using StencilOperation = RenderState::Stencil::Operation;
    switch(stencil_operation)
    {
    case StencilOperation::Keep:            return vk::StencilOp::eKeep;
    case StencilOperation::Zero:            return vk::StencilOp::eZero;
    case StencilOperation::Replace:         return vk::StencilOp::eReplace;
    case StencilOperation::Invert:          return vk::StencilOp::eInvert;
    case StencilOperation::IncrementClamp:  return vk::StencilOp::eIncrementAndClamp;
    case StencilOperation::DecrementClamp:  return vk::StencilOp::eDecrementAndClamp;
    case StencilOperation::IncrementWrap:   return vk::StencilOp::eIncrementAndWrap;
    case StencilOperation::DecrementWrap:   return vk::StencilOp::eDecrementAndWrap;
    default:
        META_UNEXPECTED_ARG_RETURN(stencil_operation, vk::StencilOp::eKeep);
    }
}

[[nodiscard]]
static vk::BlendFactor BlendingFactorToVulkan(RenderState::Blending::Factor blend_factor)
{
    META_FUNCTION_TASK();
    using BlendFactor = RenderState::Blending::Factor;
    switch(blend_factor)
    {
    case BlendFactor::Zero:                     return vk::BlendFactor::eZero;
    case BlendFactor::One:                      return vk::BlendFactor::eOne;
    case BlendFactor::SourceColor:              return vk::BlendFactor::eSrcColor;
    case BlendFactor::OneMinusSourceColor:      return vk::BlendFactor::eOneMinusSrcColor;
    case BlendFactor::SourceAlpha:              return vk::BlendFactor::eSrcAlpha;
    case BlendFactor::OneMinusSourceAlpha:      return vk::BlendFactor::eOneMinusSrcAlpha;
    case BlendFactor::DestinationColor:         return vk::BlendFactor::eDstColor;
    case BlendFactor::OneMinusDestinationColor: return vk::BlendFactor::eOneMinusDstColor;
    case BlendFactor::DestinationAlpha:         return vk::BlendFactor::eDstAlpha;
    case BlendFactor::OneMinusDestinationAlpha: return vk::BlendFactor::eOneMinusDstAlpha;
    case BlendFactor::SourceAlphaSaturated:     return vk::BlendFactor::eSrcAlphaSaturate;
    case BlendFactor::BlendColor:               return vk::BlendFactor::eConstantColor;
    case BlendFactor::OneMinusBlendColor:       return vk::BlendFactor::eOneMinusConstantColor;
    case BlendFactor::BlendAlpha:               return vk::BlendFactor::eConstantAlpha;
    case BlendFactor::OneMinusBlendAlpha:       return vk::BlendFactor::eOneMinusConstantAlpha;
    case BlendFactor::Source1Color:             return vk::BlendFactor::eSrc1Color;
    case BlendFactor::OneMinusSource1Color:     return vk::BlendFactor::eOneMinusSrc1Color;
    case BlendFactor::Source1Alpha:             return vk::BlendFactor::eSrc1Alpha;
    case BlendFactor::OneMinusSource1Alpha:     return vk::BlendFactor::eOneMinusSrc1Alpha;
    default:
        META_UNEXPECTED_ARG_RETURN(blend_factor, vk::BlendFactor::eZero);
    }
}

[[nodiscard]]
vk::BlendOp BlendingOperationToVulkan(RenderState::Blending::Operation blend_operation)
{
    META_FUNCTION_TASK();
    using BlendOperation = RenderState::Blending::Operation;
    switch(blend_operation)
    {
    case BlendOperation::Add:               return vk::BlendOp::eAdd;
    case BlendOperation::Subtract:          return vk::BlendOp::eSubtract;
    case BlendOperation::ReverseSubtract:   return vk::BlendOp::eReverseSubtract;
    case BlendOperation::Minimum:           return vk::BlendOp::eMin;
    case BlendOperation::Maximum:           return vk::BlendOp::eMax;
    default:
        META_UNEXPECTED_ARG_RETURN(blend_operation, vk::BlendOp::eAdd);
    }
}

[[nodiscard]]
vk::ColorComponentFlags BlendingColorChannelsToVulkan(RenderState::Blending::ColorChannels color_channels)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;
    using ColorChannels = RenderState::Blending::ColorChannels;

    vk::ColorComponentFlags color_component_flags{};
    if (static_cast<bool>(color_channels & ColorChannels::Red))
        color_component_flags |= vk::ColorComponentFlagBits::eR;

    if (static_cast<bool>(color_channels & ColorChannels::Green))
        color_component_flags |= vk::ColorComponentFlagBits::eG;

    if (static_cast<bool>(color_channels & ColorChannels::Blue))
        color_component_flags |= vk::ColorComponentFlagBits::eB;

    if (static_cast<bool>(color_channels & ColorChannels::Alpha))
        color_component_flags |= vk::ColorComponentFlagBits::eA;

    return color_component_flags;
}

[[nodiscard]]
static vk::Viewport ViewportToVulkan(const Viewport& viewport) noexcept
{
    META_FUNCTION_TASK();
    return vk::Viewport(
        static_cast<float>(viewport.origin.GetX()), static_cast<float>(viewport.origin.GetY()),
        static_cast<float>(viewport.size.GetWidth()), static_cast<float>(viewport.size.GetHeight()),
        static_cast<float>(viewport.origin.GetZ()), static_cast<float>(viewport.origin.GetZ() + viewport.size.GetDepth())
    );
}

[[nodiscard]]
static vk::Rect2D ScissorRectToVulkan(const ScissorRect& scissor_rect) noexcept
{
    META_FUNCTION_TASK();
    return vk::Rect2D(
        vk::Offset2D(static_cast<int32_t>(scissor_rect.origin.GetX()), static_cast<int32_t>(scissor_rect.origin.GetY())),
        vk::Extent2D(scissor_rect.size.GetWidth(), scissor_rect.size.GetHeight())
    );
}

[[nodiscard]]
static std::vector<vk::Viewport> ViewportsToVulkan(const Viewports& viewports) noexcept
{
    META_FUNCTION_TASK();
    std::vector<vk::Viewport> vk_viewports;
    std::transform(viewports.begin(), viewports.end(), std::back_inserter(vk_viewports),
                   [](const Viewport& viewport) { return ViewportToVulkan(viewport); });
    return vk_viewports;
}

[[nodiscard]]
static std::vector<vk::Rect2D> ScissorRectsToVulkan(const ScissorRects& scissor_rects) noexcept
{
    META_FUNCTION_TASK();
    std::vector<vk::Rect2D> vk_scissor_rects;
    std::transform(scissor_rects.begin(), scissor_rects.end(), std::back_inserter(vk_scissor_rects),
                   [](const ScissorRect& scissor_rect) { return ScissorRectToVulkan(scissor_rect); });
    return vk_scissor_rects;
}

Ptr<ViewState> ViewState::Create(const ViewState::Settings& state_settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<ViewStateVK>(state_settings);
}

ViewStateVK::ViewStateVK(const Settings& settings)
    : ViewStateBase(settings)
    , m_vk_viewports(ViewportsToVulkan(settings.viewports))
    , m_vk_scissor_rects(ScissorRectsToVulkan(settings.scissor_rects))
{
    META_FUNCTION_TASK();
}

bool ViewStateVK::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    if (!ViewStateBase::Reset(settings))
        return false;

    m_vk_viewports     = ViewportsToVulkan(settings.viewports);
    m_vk_scissor_rects = ScissorRectsToVulkan(settings.scissor_rects);
    return true;
}

bool ViewStateVK::SetViewports(const Viewports& viewports)
{
    META_FUNCTION_TASK();
    if (!ViewStateBase::SetViewports(viewports))
        return false;

    m_vk_viewports = ViewportsToVulkan(GetSettings().viewports);
    return true;
}

bool ViewStateVK::SetScissorRects(const ScissorRects& scissor_rects)
{
    META_FUNCTION_TASK();
    if (!ViewStateBase::SetScissorRects(scissor_rects))
        return false;

    m_vk_scissor_rects = ScissorRectsToVulkan(GetSettings().scissor_rects);
    return true;
}

void ViewStateVK::Apply(RenderCommandListBase& command_list)
{
    META_FUNCTION_TASK();
    const vk::CommandBuffer& vk_command_buffer = static_cast<RenderCommandListVK&>(command_list).GetNativeCommandBufferDefault();
    vk_command_buffer.setViewportWithCountEXT(m_vk_viewports);
    vk_command_buffer.setScissorWithCountEXT(m_vk_scissor_rects);
}

Ptr<RenderState> RenderState::Create(const RenderContext& context, const RenderState::Settings& state_settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderStateVK>(dynamic_cast<const RenderContextBase&>(context), state_settings);
}

RenderStateVK::RenderStateVK(const RenderContextBase& context, const Settings& settings)
    : RenderStateBase(context, settings)
{
    META_FUNCTION_TASK();
    Reset(settings);
}

void RenderStateVK::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    RenderStateBase::Reset(settings);

    vk::PipelineRasterizationStateCreateInfo rasterizer_info(
        vk::PipelineRasterizationStateCreateFlags{},
        false, // depthClampEnable
        false, // rasterizerDiscardEnable
        RasterizerFillModeToVulkan(settings.rasterizer.fill_mode),
        RasterizerCullModeToVulkan(settings.rasterizer.cull_mode),
        settings.rasterizer.is_front_counter_clockwise ? vk::FrontFace::eCounterClockwise : vk::FrontFace::eClockwise,
        false, // depthBiasEnable
        0.F,   // depthBiasConstantFactor
        0.F,   // depthBiasClamp
        0.F,   // depthBiasSlopeFactor
        1.F    // lineWidth
    );

    vk::PipelineMultisampleStateCreateInfo multisample_info(
        vk::PipelineMultisampleStateCreateFlags{},
        RasterizerSampleCountToVulkan(settings.rasterizer.sample_count),
        false,   // sampleShadingEnable
        1.F,     // minSampleShading
        nullptr, // pSampleMask
        settings.rasterizer.alpha_to_coverage_enabled,
        false    // alphaToOneEnable
    );

    vk::PipelineDepthStencilStateCreateInfo depth_stencil_info(
        vk::PipelineDepthStencilStateCreateFlags{},
        settings.depth.enabled,
        settings.depth.write_enabled,
        TypeConverterVK::CompareFunctionToVulkan(settings.depth.compare),
        false, // depthBoundsTestEnable
        settings.stencil.enabled,
        vk::StencilOpState( // front face
            StencilOperationToVulkan(settings.stencil.front_face.stencil_failure),
            StencilOperationToVulkan(settings.stencil.front_face.stencil_pass),
            StencilOperationToVulkan(settings.stencil.front_face.depth_failure),
            TypeConverterVK::CompareFunctionToVulkan(settings.stencil.front_face.compare),
            0U, // compareMask
            0U, // writeMask
            0U  // reference
        ),
        vk::StencilOpState( // back face
            StencilOperationToVulkan(settings.stencil.back_face.stencil_failure),
            StencilOperationToVulkan(settings.stencil.back_face.stencil_pass),
            StencilOperationToVulkan(settings.stencil.back_face.depth_failure),
            TypeConverterVK::CompareFunctionToVulkan(settings.stencil.back_face.compare),
            0U, // compareMask
            0U, // writeMask
            0U  // reference
        ),
        0.F, // minDepthBounds
        0.F  // maxDepthBounds
    );

    const size_t blend_attachments_count = settings.blending.is_independent
                                         ? settings.program_ptr->GetSettings().attachment_formats.colors.size()
                                         : 1;
    std::vector<vk::PipelineColorBlendAttachmentState> attachment_blend_states;
    std::transform(settings.blending.render_targets.begin(),
                   settings.blending.render_targets.begin() + blend_attachments_count,
                   std::back_inserter(attachment_blend_states),
        [](const Blending::RenderTarget& rt_blending)
        {
            return vk::PipelineColorBlendAttachmentState(
                rt_blending.blend_enabled,
                BlendingFactorToVulkan(rt_blending.source_rgb_blend_factor),
                BlendingFactorToVulkan(rt_blending.dest_rgb_blend_factor),
                BlendingOperationToVulkan(rt_blending.rgb_blend_op),
                BlendingFactorToVulkan(rt_blending.source_alpha_blend_factor),
                BlendingFactorToVulkan(rt_blending.dest_alpha_blend_factor),
                BlendingOperationToVulkan(rt_blending.alpha_blend_op),
                BlendingColorChannelsToVulkan(rt_blending.write_mask)
            );
        }
    );

    vk::PipelineColorBlendStateCreateInfo blending_info(
        vk::PipelineColorBlendStateCreateFlags{},
        false, // logicOpEnable
        vk::LogicOp::eCopy,
        attachment_blend_states,
        settings.blending_color.AsArray()
    );

    // Fake state, actual PrimitiveTopology is set dynamically
    vk::PipelineInputAssemblyStateCreateInfo assembly_info(
        vk::PipelineInputAssemblyStateCreateFlags{},
        vk::PrimitiveTopology::eTriangleList,
        false
    );

    // Fake viewport state, actual state is set dynamically
    vk::PipelineViewportStateCreateInfo viewport_info(
        vk::PipelineViewportStateCreateFlags{},
        0, nullptr, 0, nullptr
    );

    const std::vector<vk::DynamicState> dynamic_states = {
        vk::DynamicState::eViewportWithCountEXT,
        vk::DynamicState::eScissorWithCountEXT,
        vk::DynamicState::ePrimitiveTopologyEXT,
    };
    vk::PipelineDynamicStateCreateInfo dynamic_info(
        vk::PipelineDynamicStateCreateFlags{},
        dynamic_states
    );

    auto& program = static_cast<ProgramVK&>(*GetSettings().program_ptr);
    const auto& render_pattern = static_cast<RenderPatternVK&>(*GetSettings().render_pattern_ptr);

    const vk::PipelineVertexInputStateCreateInfo vk_vertex_input_state_info = program.GetNativeVertexInputStateCreateInfo();
    const std::vector<vk::PipelineShaderStageCreateInfo> vk_stages_info = program.GetNativeShaderStageCreateInfos();

    const vk::GraphicsPipelineCreateInfo vk_pipeline_create_info(
        vk::PipelineCreateFlags(),
        vk_stages_info,
        &vk_vertex_input_state_info,
        &assembly_info,
        nullptr, // no tesselation support yet
        &viewport_info,
        &rasterizer_info,
        &multisample_info,
        &depth_stencil_info,
        &blending_info,
        &dynamic_info,
        program.GetNativePipelineLayout(),
        render_pattern.GetNativeRenderPass()
    );

    auto pipe = GetContextVK().GetDeviceVK().GetNativeDevice().createGraphicsPipelineUnique(nullptr, vk_pipeline_create_info);
    META_CHECK_ARG_EQUAL_DESCR(pipe.result, vk::Result::eSuccess, "Vulkan pipeline creation has failed");
    m_vk_unique_pipeline = std::move(pipe.value);
}

void RenderStateVK::Apply(RenderCommandListBase& render_command_list, Groups /*state_groups*/)
{
    META_FUNCTION_TASK();
    const auto& vulkan_render_command_list = static_cast<RenderCommandListVK&>(render_command_list);
    vulkan_render_command_list.GetNativeCommandBufferDefault().bindPipeline(vk::PipelineBindPoint::eGraphics, GetNativePipeline());
}

bool RenderStateVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!RenderStateBase::SetName(name))
        return false;

    SetVulkanObjectName(GetContextVK().GetDeviceVK().GetNativeDevice(), m_vk_unique_pipeline.get(), name.c_str());
    return true;
}

const IContextVK& RenderStateVK::GetContextVK() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const IContextVK&>(GetRenderContext());
}

} // namespace Methane::Graphics
