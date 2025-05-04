/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Base/RenderPattern.cpp
Base implementation of the render pattern interface.

******************************************************************************/

#include <Methane/Graphics/Base/RenderPattern.h>
#include <Methane/Graphics/Base/RenderContext.h>

#include <Methane/Instrumentation.h>

#include <algorithm>
#include <iterator>

namespace Methane::Graphics::Base
{

RenderPattern::RenderPattern(RenderContext& render_context, const Settings& settings)
    : m_render_context_ptr(render_context.GetDerivedPtr<RenderContext>())
    , m_settings(settings)
{ }

const Rhi::IRenderContext& RenderPattern::GetRenderContext() const noexcept
{
    META_FUNCTION_TASK();
    return *m_render_context_ptr;
}

Rhi::IRenderContext& RenderPattern::GetRenderContext() noexcept
{
    META_FUNCTION_TASK();
    return *m_render_context_ptr;
}

Data::Size RenderPattern::GetAttachmentCount() const noexcept
{
    META_FUNCTION_TASK();
    auto attachment_count = static_cast<Data::Size>(m_settings.color_attachments.size());
    if (m_settings.depth_attachment)
        attachment_count++;
    if (m_settings.stencil_attachment)
        attachment_count++;
    return attachment_count;
}

AttachmentFormats RenderPattern::GetAttachmentFormats() const noexcept
{
    META_FUNCTION_TASK();
    AttachmentFormats attachment_formats;

    attachment_formats.colors.reserve(m_settings.color_attachments.size());
    std::ranges::transform(m_settings.color_attachments, std::back_inserter(attachment_formats.colors),
                   [](const ColorAttachment& color_attachment) { return color_attachment.format; });

    if (m_settings.depth_attachment)
        attachment_formats.depth = m_settings.depth_attachment->format;

    if (m_settings.stencil_attachment)
        attachment_formats.stencil = m_settings.stencil_attachment->format;

    return attachment_formats;
}

} // namespace Methane::Graphics::Base
