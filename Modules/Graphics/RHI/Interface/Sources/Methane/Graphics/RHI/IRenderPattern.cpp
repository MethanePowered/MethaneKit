/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/IRenderPattern.cpp
Methane render pattern interface.

******************************************************************************/

#include <Methane/Graphics/RHI/IRenderPattern.h>
#include <Methane/Graphics/RHI/IRenderContext.h>

#include <Methane/Data/EnumMaskUtil.hpp>
#include <Methane/Instrumentation.h>

#include <fmt/format.h>

template<>
struct fmt::formatter<Methane::Graphics::Rhi::RenderPassColorAttachment>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Graphics::Rhi::RenderPassColorAttachment& ca, FormatContext& ctx) const
    { return format_to(ctx.out(), "{}", static_cast<std::string>(ca)); }

    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const
    { return ctx.end(); }
};

namespace Methane::Graphics::Rhi
{

RenderPassAttachment::RenderPassAttachment(Data::Index attachment_index, PixelFormat format,
                                           Data::Size samples_count, LoadAction load_action, StoreAction store_action)
    : attachment_index(attachment_index)
    , format(format)
    , samples_count(samples_count)
    , load_action(load_action)
    , store_action(store_action)
{ }

bool RenderPassAttachment::operator==(const RenderPassAttachment& other) const
{
    META_FUNCTION_TASK();
    return std::tie(attachment_index, format, samples_count, load_action, store_action) ==
           std::tie(other.attachment_index, format, samples_count, other.load_action, other.store_action);
}

bool RenderPassAttachment::operator!=(const RenderPassAttachment& other) const
{
    META_FUNCTION_TASK();
    return std::tie(attachment_index, format, samples_count, load_action, store_action) !=
           std::tie(other.attachment_index, format, samples_count, other.load_action, other.store_action);
}

RenderPassAttachment::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("attachment id {}: format={}, samples={}, load={}, store={}",
                       attachment_index,
                       magic_enum::enum_name(format),
                       samples_count,
                       magic_enum::enum_name(load_action),
                       magic_enum::enum_name(store_action));
}

RenderPassColorAttachment::RenderPassColorAttachment(Data::Index attachment_index, PixelFormat format, Data::Size samples_count,
                                                     LoadAction load_action, StoreAction store_action, const Color4F& clear_color)
    : RenderPassAttachment(attachment_index, format, samples_count, load_action, store_action)
    , clear_color(clear_color)
{ }

bool RenderPassColorAttachment::operator==(const RenderPassColorAttachment& other) const
{
    META_FUNCTION_TASK();
    return RenderPassAttachment::operator==(other) && clear_color == other.clear_color;
}

bool RenderPassColorAttachment::operator!=(const RenderPassColorAttachment& other) const
{
    META_FUNCTION_TASK();
    return RenderPassAttachment::operator!=(other) || clear_color != other.clear_color;
}

RenderPassColorAttachment::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("  - Color {}, clear_color={}",
                       RenderPassAttachment::operator std::string(),
                       static_cast<std::string>(clear_color));
}

RenderPassDepthAttachment::RenderPassDepthAttachment(Data::Index attachment_index, PixelFormat format, Data::Size samples_count,
                                                     LoadAction load_action, StoreAction store_action, Depth clear_value)
    : RenderPassAttachment(attachment_index, format, samples_count, load_action, store_action)
    , clear_value(clear_value)
{ }

bool RenderPassDepthAttachment::operator==(const RenderPassDepthAttachment& other) const
{
    META_FUNCTION_TASK();
    return RenderPassAttachment::operator==(other) && clear_value == other.clear_value;
}

bool RenderPassDepthAttachment::operator!=(const RenderPassDepthAttachment& other) const
{
    META_FUNCTION_TASK();
    return RenderPassAttachment::operator!=(other) || clear_value != other.clear_value;
}

RenderPassDepthAttachment::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("  - Depth {}, clear_value={}",
                       RenderPassAttachment::operator std::string(),
                       clear_value);
}

RenderPassStencilAttachment::RenderPassStencilAttachment(Data::Index attachment_index, PixelFormat format, Data::Size samples_count,
                                                         LoadAction load_action, StoreAction store_action, Stencil clear_value)
    : RenderPassAttachment(attachment_index, format, samples_count, load_action, store_action)
    , clear_value(clear_value)
{ }

bool RenderPassStencilAttachment::operator==(const RenderPassStencilAttachment& other) const
{
    META_FUNCTION_TASK();
    return RenderPassAttachment::operator==(other) && clear_value == other.clear_value;
}

bool RenderPassStencilAttachment::operator!=(const RenderPassStencilAttachment& other) const
{
    META_FUNCTION_TASK();
    return RenderPassAttachment::operator!=(other) || clear_value != other.clear_value;
}

RenderPassStencilAttachment::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("  - Stencil {}, clear_value={}",
                       RenderPassAttachment::operator std::string(),
                       clear_value);
}

bool RenderPatternSettings::operator==(const RenderPatternSettings& other) const
{
    META_FUNCTION_TASK();
    return std::tie(color_attachments, depth_attachment, stencil_attachment, shader_access, is_final_pass) ==
           std::tie(other.color_attachments, other.depth_attachment, other.stencil_attachment, other.shader_access, other.is_final_pass);
}

bool RenderPatternSettings::operator!=(const RenderPatternSettings& other) const
{
    META_FUNCTION_TASK();
    return std::tie(color_attachments, depth_attachment, stencil_attachment, shader_access, is_final_pass) !=
           std::tie(other.color_attachments, other.depth_attachment, other.stencil_attachment, other.shader_access, other.is_final_pass);
}

RenderPatternSettings::operator std::string() const
{
    META_FUNCTION_TASK();
    std::string color_attachments_str = "  - No color attachments";
    if (!color_attachments.empty())
        color_attachments_str = fmt::format("{}", fmt::join(color_attachments, ";\n"));

    return fmt::format("{};\n{};\n{};\n  - shader_access={}, {} pass.",
                       color_attachments_str,
                       depth_attachment   ? static_cast<std::string>(*depth_attachment)   : "  - No stencil attachment",
                       stencil_attachment ? static_cast<std::string>(*stencil_attachment) : "  - No depth attachment",
                       Data::GetEnumMaskName(shader_access),
                       (is_final_pass ? "final" : "intermediate"));
}

Ptr<IRenderPattern> IRenderPattern::Create(IRenderContext& render_context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return render_context.CreateRenderPattern(settings);
}

} // namespace Methane::Graphics::Rhi
