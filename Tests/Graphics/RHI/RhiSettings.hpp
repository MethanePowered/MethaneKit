/******************************************************************************

Copyright 2025 Evgeny Gorodetskiy

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

FILE: Tests/Graphics/RHI/RenderStateSettings.hpp
Test settings of the RHI RenderStateSettings

******************************************************************************/

#include <Methane/Data/AppShadersProvider.h>
#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/RenderState.h>

namespace Methane::Graphics::Test
{

inline Rhi::RenderContextSettings GetRenderContextSettings()
{
    return Rhi::RenderContextSettings
    {
        .frame_size           = { 1920U, 1080U },
        .color_format         = PixelFormat::BGRA8Unorm,
        .depth_stencil_format = PixelFormat::Depth32Float,
        .clear_color          = Color4F{ 0.f, 0.f, 1.f, 1.f },
        .clear_depth_stencil  = DepthStencilValues{ 0.f, 0.f },
        .frame_buffers_count  = 2U,
        .vsync_enabled        = false,
        .is_full_screen       = true,
        .options_mask         = Rhi::ContextOptionMask{ Rhi::ContextOption::DeferredProgramBindingsInitialization },
        .unsync_max_fps       = 1234U
    };
}

inline Rhi::RenderPatternSettings GetRenderPatternSettings()
{
    return Rhi::RenderPatternSettings
    {
        .color_attachments = Rhi::RenderPassColorAttachments{
            Rhi::RenderPassColorAttachment{
                0U, PixelFormat::RGBA8Unorm_sRGB, 1U,
                Rhi::RenderPassAttachment::LoadAction::Clear,
                Rhi::RenderPassAttachment::StoreAction::Store,
                Color4F(0.1F, 0.2F, 0.3F, 1.F)
            }
        },
        .depth_attachment = Rhi::RenderPassDepthAttachment{
            1U, PixelFormat::Depth32Float, 1U,
            Rhi::RenderPassAttachment::LoadAction::Clear,
            Rhi::RenderPassAttachment::StoreAction::Store,
            0.F
        },
        .stencil_attachment = std::nullopt,
        .shader_access = Rhi::RenderPassAccessMask{ Rhi::RenderPassAccess::ShaderResources },
        .is_final_pass = true
    };
}

inline Rhi::RenderStateSettingsImpl GetRenderStateSettings(const Rhi::RenderContext& render_context,
                                                           const Rhi::RenderPattern& render_pattern)
{
    return Rhi::RenderStateSettingsImpl
    {
        .program = render_context.CreateProgram(
            Rhi::Program::Settings
            {
                .shader_set = Rhi::Program::ShaderSet
                {
                    { Rhi::ShaderType::Vertex, { Data::ShaderProvider::Get(), { "Shader", "MainVS" } } },
                    { Rhi::ShaderType::Pixel,  { Data::ShaderProvider::Get(), { "Shader", "MainPS" } } }
                },
                .attachment_formats = AttachmentFormats
                {
                    .colors  = { PixelFormat::RGBA8Unorm_sRGB },
                    .depth   = PixelFormat::Depth32Float,
                    .stencil = PixelFormat::Unknown
                }
            }),
        .render_pattern = render_pattern,
        .rasterizer = Rhi::RasterizerSettings
        {
            .is_front_counter_clockwise = true,
            .cull_mode                  = Rhi::RasterizerCullMode::Front,
            .fill_mode                  = Rhi::RasterizerFillMode::Solid,
            .sample_count               = 1,
            .alpha_to_coverage_enabled  = false
        },
        .depth = Rhi::DepthSettings
        {
            .enabled       = true,
            .write_enabled = true,
            .compare       = Compare::LessEqual,
        },
        .stencil = Rhi::StencilSettings
        {
            .enabled    = false,
            .read_mask  = static_cast<uint8_t>(~0x0),
            .write_mask = static_cast<uint8_t>(~0x0),
            .front_face = Rhi::FaceOperations
            {
                .stencil_failure    = Rhi::FaceOperation::Keep,
                .stencil_pass       = Rhi::FaceOperation::Keep,
                .depth_failure      = Rhi::FaceOperation::Keep,
                .depth_stencil_pass = Rhi::FaceOperation::Keep,
                .compare            = Compare::Always,
            },
            .back_face = Rhi::FaceOperations
            {
                .stencil_failure    = Rhi::FaceOperation::Keep,
                .stencil_pass       = Rhi::FaceOperation::Keep,
                .depth_failure      = Rhi::FaceOperation::Keep,
                .depth_stencil_pass = Rhi::FaceOperation::Keep,
                .compare            = Compare::Always,
            }
        },
        .blending = Rhi::BlendingSettings
        {
            .is_independent = false,
            .render_targets = {
                Rhi::RenderTargetSettings
                {
                    .blend_enabled             = false,
                    .color_write               = Rhi::BlendingColorChannelMask{ ~0U },
                    .rgb_blend_op              = Rhi::BlendingOperation::Add,
                    .alpha_blend_op            = Rhi::BlendingOperation::Add,
                    .source_rgb_blend_factor   = Rhi::BlendingFactor::One,
                    .source_alpha_blend_factor = Rhi::BlendingFactor::One,
                    .dest_rgb_blend_factor     = Rhi::BlendingFactor::Zero,
                    .dest_alpha_blend_factor   = Rhi::BlendingFactor::Zero
                }
            }
        },
        .blending_color = Color4F{ 1.f, 1.f, 1.f, 1.f }
    };
}

} // namespace Methane::Graphics
