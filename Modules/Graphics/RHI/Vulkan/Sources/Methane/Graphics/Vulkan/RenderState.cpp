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

FILE: Methane/Graphics/Vulkan/RenderState.cpp
Vulkan implementation of the render state interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/RenderContext.h>
#include <Methane/Graphics/Vulkan/RenderState.h>
#include <Methane/Graphics/Vulkan/RenderPattern.h>
#include <Methane/Graphics/Vulkan/IContext.h>
#include <Methane/Graphics/Vulkan/Device.h>
#include <Methane/Graphics/Vulkan/RenderCommandList.h>
#include <Methane/Graphics/Vulkan/Program.h>
#include <Methane/Graphics/Vulkan/Shader.h>
#include <Methane/Graphics/Vulkan/ViewState.h>
#include <Methane/Graphics/Vulkan/Types.h>
#include <Methane/Graphics/Vulkan/Utils.hpp>

#include <Methane/Graphics/Base/RenderContext.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <algorithm>
#include <vulkan/vulkan_enums.hpp>

namespace Methane::Graphics::Vulkan
{

[[nodiscard]]
static vk::PolygonMode RasterizerFillModeToVulkan(Rhi::IRenderState::Rasterizer::FillMode fill_mode)
{
    META_FUNCTION_TASK();
    switch(fill_mode)
    {
    using enum Rhi::IRenderState::Rasterizer::FillMode;
    using enum vk::PolygonMode;
    case Solid:     return eFill;
    case Wireframe: return eLine;
    default:
        META_UNEXPECTED_RETURN(fill_mode, vk::PolygonMode::eFill);
    }
}

[[nodiscard]]
static vk::CullModeFlags RasterizerCullModeToVulkan(Rhi::IRenderState::Rasterizer::CullMode cull_mode)
{
    META_FUNCTION_TASK();
    switch(cull_mode)
    {
    using enum Rhi::IRenderState::Rasterizer::CullMode;
    using enum vk::CullModeFlagBits;
    case None:  return eNone;
    case Back:  return eBack;
    case Front: return eFront;
    default:
        META_UNEXPECTED_RETURN(cull_mode, vk::CullModeFlagBits::eNone);
    }
}

[[nodiscard]]
static vk::StencilOp StencilOperationToVulkan(Rhi::FaceOperation stencil_operation)
{
    META_FUNCTION_TASK();
    switch(stencil_operation)
    {
    using enum Rhi::FaceOperation;
    using enum vk::StencilOp;
    case Keep:            return eKeep;
    case Zero:            return eZero;
    case Replace:         return eReplace;
    case Invert:          return eInvert;
    case IncrementClamp:  return eIncrementAndClamp;
    case DecrementClamp:  return eDecrementAndClamp;
    case IncrementWrap:   return eIncrementAndWrap;
    case DecrementWrap:   return eDecrementAndWrap;
    default:
        META_UNEXPECTED_RETURN(stencil_operation, vk::StencilOp::eKeep);
    }
}

[[nodiscard]]
static vk::BlendFactor BlendingFactorToVulkan(Rhi::IRenderState::Blending::Factor blend_factor)
{
    META_FUNCTION_TASK();
    switch(blend_factor)
    {
    using enum Rhi::IRenderState::Blending::Factor;
    using enum vk::BlendFactor;
    case Zero:                     return eZero;
    case One:                      return eOne;
    case SourceColor:              return eSrcColor;
    case OneMinusSourceColor:      return eOneMinusSrcColor;
    case SourceAlpha:              return eSrcAlpha;
    case OneMinusSourceAlpha:      return eOneMinusSrcAlpha;
    case DestinationColor:         return eDstColor;
    case OneMinusDestinationColor: return eOneMinusDstColor;
    case DestinationAlpha:         return eDstAlpha;
    case OneMinusDestinationAlpha: return eOneMinusDstAlpha;
    case SourceAlphaSaturated:     return eSrcAlphaSaturate;
    case BlendColor:               return eConstantColor;
    case OneMinusBlendColor:       return eOneMinusConstantColor;
    case BlendAlpha:               return eConstantAlpha;
    case OneMinusBlendAlpha:       return eOneMinusConstantAlpha;
    case Source1Color:             return eSrc1Color;
    case OneMinusSource1Color:     return eOneMinusSrc1Color;
    case Source1Alpha:             return eSrc1Alpha;
    case OneMinusSource1Alpha:     return eOneMinusSrc1Alpha;
    default: META_UNEXPECTED_RETURN(blend_factor, vk::BlendFactor::eZero);
    }
}

[[nodiscard]]
static vk::BlendOp BlendingOperationToVulkan(Rhi::IRenderState::Blending::Operation blend_operation)
{
    META_FUNCTION_TASK();
    using BlendOperation = Rhi::IRenderState::Blending::Operation;
    switch(blend_operation)
    {
    using enum BlendOperation;
    using enum vk::BlendOp;
    case Add:               return eAdd;
    case Subtract:          return eSubtract;
    case ReverseSubtract:   return eReverseSubtract;
    case Minimum:           return eMin;
    case Maximum:           return eMax;
    default:
        META_UNEXPECTED_RETURN(blend_operation, vk::BlendOp::eAdd);
    }
}

[[nodiscard]]
static vk::ColorComponentFlags BlendingColorChannelsToVulkan(Rhi::BlendingColorChannelMask color_channels)
{
    META_FUNCTION_TASK();
    using enum Rhi::BlendingColorChannel;
    using enum vk::ColorComponentFlagBits;
    vk::ColorComponentFlags color_component_flags{};

    if (color_channels.HasAnyBit(Red))
        color_component_flags |= eR;

    if (color_channels.HasAnyBit(Green))
        color_component_flags |= eG;

    if (color_channels.HasAnyBit(Blue))
        color_component_flags |= eB;

    if (color_channels.HasAnyBit(Alpha))
        color_component_flags |= eA;

    return color_component_flags;
}

vk::PrimitiveTopology RenderState::GetVulkanPrimitiveTopology(Rhi::RenderPrimitive primitive_type)
{
    META_FUNCTION_TASK();
    switch(primitive_type)
    {
    using enum Rhi::RenderPrimitive;
    using enum vk::PrimitiveTopology;
    case Point:           return ePointList;
    case Line:            return eLineList;
    case LineStrip:       return eLineStrip;
    case Triangle:        return eTriangleList;
    case TriangleStrip:   return eTriangleStrip;
    default: META_UNEXPECTED_RETURN(primitive_type, vk::PrimitiveTopology::ePointList);
    }
}

RenderState::RenderState(const Base::RenderContext& context, const Settings& settings)
    : Base::RenderState(context, settings,
                        !dynamic_cast<const IContext&>(context).GetVulkanDevice().IsDynamicStateSupported())
    , m_vk_render_context(static_cast<const RenderContext&>(GetRenderContext()))
{
    META_FUNCTION_TASK();
    Reset(settings);
}

void RenderState::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    Base::RenderState::Reset(settings);

    if (IsNativePipelineDynamic())
    {
        m_vk_pipeline_dynamic = CreateNativePipeline();
    }
    else
    {
        m_vk_pipeline_monolithic_by_id.clear();
    }
}

void RenderState::Apply(Base::RenderCommandList& render_command_list, Groups /*state_groups*/)
{
    META_FUNCTION_TASK();
    const auto& vulkan_render_command_list = static_cast<RenderCommandList&>(render_command_list);
    const vk::Pipeline& vk_pipeline_state = IsNativePipelineDynamic()
                                          ? GetNativePipelineDynamic()
                                          : GetNativePipelineMonolithic(vulkan_render_command_list.GetDrawingState());
    vulkan_render_command_list.GetNativeCommandBufferDefault().bindPipeline(vk::PipelineBindPoint::eGraphics, vk_pipeline_state);
}

bool RenderState::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Base::RenderState::SetName(name))
        return false;

    if (IsNativePipelineDynamic())
    {
        SetVulkanObjectName(m_vk_render_context.GetVulkanDevice().GetNativeDevice(), m_vk_pipeline_dynamic.get(), name);
    }
    else
    {
        for(const auto& [pipeline_id, vk_pipeline_monolithic] : m_vk_pipeline_monolithic_by_id)
        {
            SetVulkanObjectName(m_vk_render_context.GetVulkanDevice().GetNativeDevice(), vk_pipeline_monolithic.get(), name);
        }
    }
    return true;
}

const vk::Pipeline& RenderState::GetNativePipelineDynamic() const
{
    META_FUNCTION_TASK();
    META_CHECK_TRUE_DESCR(IsNativePipelineDynamic(), "dynamic pipeline is not supported by device");
    return m_vk_pipeline_dynamic.get();
}

const vk::Pipeline& RenderState::GetNativePipelineMonolithic(ViewState& view_state, Rhi::RenderPrimitive render_primitive)
{
    META_FUNCTION_TASK();
    META_CHECK_FALSE_DESCR(IsNativePipelineDynamic(), "dynamic pipeline should be used");
    std::lock_guard lock(m_mutex);

    const PipelineId pipeline_id(static_cast<Rhi::IViewState*>(&view_state), render_primitive);
    const auto pipeline_monolithic_by_id_it = m_vk_pipeline_monolithic_by_id.find(pipeline_id);
    if (pipeline_monolithic_by_id_it == m_vk_pipeline_monolithic_by_id.end())
    {
        view_state.Connect(*this);
        return m_vk_pipeline_monolithic_by_id.try_emplace(pipeline_id, CreateNativePipeline(&view_state, render_primitive)).first->second.get();
    }

    return pipeline_monolithic_by_id_it->second.get();
}

const vk::Pipeline& RenderState::GetNativePipelineMonolithic(const Base::RenderDrawingState& drawing_state)
{
    META_FUNCTION_TASK();
    META_CHECK_NOT_NULL_DESCR(drawing_state.view_state_ptr, "view state is not set in render command list drawing state");
    META_CHECK_TRUE_DESCR(drawing_state.primitive_type_opt.has_value(), "primitive type is not set in render command list drawing state");
    return GetNativePipelineMonolithic(static_cast<ViewState&>(*drawing_state.view_state_ptr), drawing_state.primitive_type_opt.value());
}

vk::UniquePipeline RenderState::CreateNativePipeline(const ViewState* view_state_ptr, Opt<Rhi::RenderPrimitive> render_primitive_opt) const
{
    META_FUNCTION_TASK();
    const Settings& settings = GetSettings();

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
        TypeConverter::SampleCountToVulkan(settings.rasterizer.sample_count),
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
        TypeConverter::CompareFunctionToVulkan(settings.depth.compare),
        false, // depthBoundsTestEnable
        settings.stencil.enabled,
        vk::StencilOpState( // front face
            StencilOperationToVulkan(settings.stencil.front_face.stencil_failure),
            StencilOperationToVulkan(settings.stencil.front_face.stencil_pass),
            StencilOperationToVulkan(settings.stencil.front_face.depth_failure),
            TypeConverter::CompareFunctionToVulkan(settings.stencil.front_face.compare),
            0U, // compareMask
            0U, // writeMask
            0U  // reference
        ),
        vk::StencilOpState( // back face
            StencilOperationToVulkan(settings.stencil.back_face.stencil_failure),
            StencilOperationToVulkan(settings.stencil.back_face.stencil_pass),
            StencilOperationToVulkan(settings.stencil.back_face.depth_failure),
            TypeConverter::CompareFunctionToVulkan(settings.stencil.back_face.compare),
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
    std::ranges::transform(settings.blending.render_targets.begin(),
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
                                   BlendingColorChannelsToVulkan(rt_blending.color_write)
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

    vk::PipelineInputAssemblyStateCreateInfo assembly_info(
        vk::PipelineInputAssemblyStateCreateFlags{},
        render_primitive_opt ? GetVulkanPrimitiveTopology(*render_primitive_opt) : vk::PrimitiveTopology::eTriangleList,
        false
    );

    vk::PipelineViewportStateCreateInfo empty_viewport_info(
        vk::PipelineViewportStateCreateFlags{},
        0, nullptr, 0, nullptr
    );

    std::vector<vk::DynamicState> dynamic_states;
    if (!view_state_ptr)
    {
        dynamic_states.push_back(vk::DynamicState::eViewportWithCountEXT);
        dynamic_states.push_back(vk::DynamicState::eScissorWithCountEXT);
    }
    if (!render_primitive_opt)
    {
        dynamic_states.push_back(vk::DynamicState::ePrimitiveTopologyEXT);
    }

    vk::PipelineDynamicStateCreateInfo dynamic_info(
        vk::PipelineDynamicStateCreateFlags{},
        dynamic_states
    );

    auto& program = static_cast<Program&>(*GetSettings().program_ptr);
    const auto& render_pattern = static_cast<RenderPattern&>(*GetSettings().render_pattern_ptr);

    const vk::PipelineVertexInputStateCreateInfo vk_vertex_input_state_info = program.GetNativeVertexInputStateCreateInfo();
    const std::vector<vk::PipelineShaderStageCreateInfo> vk_stages_info = program.GetNativeShaderStageCreateInfos();

    const vk::GraphicsPipelineCreateInfo vk_pipeline_create_info(
        vk::PipelineCreateFlags(),
        vk_stages_info,
        &vk_vertex_input_state_info,
        &assembly_info,
        nullptr, // no tesselation support yet
        view_state_ptr ? &view_state_ptr->GetNativeViewportStateCreateInfo() : &empty_viewport_info,
        &rasterizer_info,
        &multisample_info,
        &depth_stencil_info,
        &blending_info,
        IsNativePipelineDynamic() ? &dynamic_info : nullptr,
        program.AcquireNativePipelineLayout(),
        render_pattern.GetNativeRenderPass()
    );

    auto pipe = m_vk_render_context.GetVulkanDevice().GetNativeDevice().createGraphicsPipelineUnique(nullptr, vk_pipeline_create_info);
    META_CHECK_EQUAL_DESCR(pipe.result, vk::Result::eSuccess, "Vulkan pipeline creation has failed");

    SetVulkanObjectName(m_vk_render_context.GetVulkanDevice().GetNativeDevice(), pipe.value.get(), Base::Object::GetName());
    return std::move(pipe.value);
}

void RenderState::OnViewStateChanged(Rhi::IViewState& view_state)
{
    META_FUNCTION_TASK();
    std::lock_guard lock(m_mutex);

    for(auto& [pipeline_id, vk_pipeline_monolithic] : m_vk_pipeline_monolithic_by_id)
        if (std::get<0>(pipeline_id) == &view_state)
        {
            m_vk_render_context.DeferredRelease(std::move(vk_pipeline_monolithic));
            vk_pipeline_monolithic = CreateNativePipeline(static_cast<const ViewState*>(&view_state), std::get<1>(pipeline_id));
        }
}

void RenderState::OnViewStateDestroyed(Rhi::IViewState& view_state)
{
    META_FUNCTION_TASK();
    std::lock_guard lock(m_mutex);

    for(auto vk_pipeline_it = m_vk_pipeline_monolithic_by_id.begin();
        vk_pipeline_it != m_vk_pipeline_monolithic_by_id.end();)
    {
        if (std::get<0>(vk_pipeline_it->first) != &view_state)
        {
            vk_pipeline_it++;
            continue;
        }

        m_vk_render_context.DeferredRelease(std::move(vk_pipeline_it->second));
        vk_pipeline_it = m_vk_pipeline_monolithic_by_id.erase(vk_pipeline_it);
    }
}

} // namespace Methane::Graphics::Vulkan
