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

FILE: Methane/Graphics/RHI/IRenderPass.cpp
Methane render pass interface: specifies output of the graphics pipeline.

******************************************************************************/

#include <Methane/Graphics/RHI/IRenderPass.h>

#include <Methane/Instrumentation.h>

#include <fmt/format.h>

template<>
struct fmt::formatter<Methane::Graphics::Rhi::RenderPassColorAttachment>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Graphics::Rhi::RenderPassColorAttachment& ca, FormatContext& ctx) { return format_to(ctx.out(), "{}", static_cast<std::string>(ca)); }
    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

namespace Methane::Graphics::Rhi
{

RenderPassAccess::RenderPassAccess() noexcept
    : mask(0U)
{
}

RenderPassAccess::RenderPassAccess(uint32_t mask) noexcept
    : mask(mask)
{
}

RenderPassAccess::RenderPassAccess(const std::initializer_list<Bit>& bits)
    : mask(0U)
{
    META_FUNCTION_TASK();
    for(Bit bit : bits)
    {
        SetBit(bit, true);
    }
}

void RenderPassAccess::SetBit(Bit bit, bool value)
{
    META_FUNCTION_TASK();
    switch(bit)
    {
    case Bit::ShaderResources: shader_resources = value; break;
    case Bit::Samplers:        samplers = value; break;
    case Bit::RenderTargets:   render_targets = value; break;
    case Bit::DepthStencil:    depth_stencil = value; break;
    default: META_UNEXPECTED_ARG(bit);
    }
}

std::vector<RenderPassAccess::Bit> RenderPassAccess::GetBits() const
{
    META_FUNCTION_TASK();
    std::vector<Bit> bits;
    if (shader_resources)
        bits.push_back(Bit::ShaderResources);
    if (samplers)
        bits.push_back(Bit::Samplers);
    if (render_targets)
        bits.push_back(Bit::RenderTargets);
    if (depth_stencil)
        bits.push_back(Bit::DepthStencil);
    return bits;
}

std::vector<std::string> RenderPassAccess::GetBitNames() const
{
    META_FUNCTION_TASK();
    const std::vector<Bit> bits = GetBits();
    std::vector<std::string> bit_names;
    for(Bit bit : bits)
    {
        bit_names.emplace_back(magic_enum::enum_name(bit));
    }
    return bit_names;
}

RenderPassAttachment::RenderPassAttachment(Data::Index attachment_index, PixelFormat format,
                                           Data::Size samples_count, LoadAction load_action, StoreAction store_action)
    : attachment_index(attachment_index)
    , format(format)
    , samples_count(samples_count)
    , load_action(load_action)
    , store_action(store_action)
{
    META_FUNCTION_TASK();
}

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
{
    META_FUNCTION_TASK();
}

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
{
    META_FUNCTION_TASK();
}

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
{
    META_FUNCTION_TASK();
}

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
    return std::tie(color_attachments, depth_attachment, stencil_attachment, shader_access.mask, is_final_pass) ==
           std::tie(other.color_attachments, other.depth_attachment, other.stencil_attachment, other.shader_access.mask, other.is_final_pass);
}

bool RenderPatternSettings::operator!=(const RenderPatternSettings& other) const
{
    META_FUNCTION_TASK();
    return std::tie(color_attachments, depth_attachment, stencil_attachment, shader_access.mask, is_final_pass) !=
           std::tie(other.color_attachments, other.depth_attachment, other.stencil_attachment, other.shader_access.mask, other.is_final_pass);
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
                       fmt::join(shader_access.GetBitNames(), "|"),
                       (is_final_pass ? "final" : "intermediate"));
}

bool RenderPassSettings::operator==(const RenderPassSettings& other) const
{
    META_FUNCTION_TASK();
    return std::tie(attachments, frame_size) ==
           std::tie(other.attachments, other.frame_size);
}

bool RenderPassSettings::operator!=(const RenderPassSettings& other) const
{
    META_FUNCTION_TASK();
    return std::tie(attachments, frame_size) !=
           std::tie(other.attachments, other.frame_size);
}

} // namespace Methane::Graphics::Rhi
