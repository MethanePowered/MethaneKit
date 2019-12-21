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

FILE: Methane/Graphics/Vulkan/RenderPassVK.mm
Vulkan implementation of the render pass interface.

******************************************************************************/

#include "RenderPassVK.h"
#include "ContextVK.h"
#include "TextureVK.h"

#include <Methane/Data/Instrumentation.h>

namespace Methane::Graphics
{

RenderPass::Ptr RenderPass::Create(Context& context, const Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<RenderPassVK>(static_cast<ContextBase&>(context), settings);
}

RenderPassVK::RenderPassVK(ContextBase& context, const Settings& settings)
    : RenderPassBase(context, settings)
{
    ITT_FUNCTION_TASK();

    Reset();
}

void RenderPassVK::Update(const Settings& settings)
{
    ITT_FUNCTION_TASK();

    m_settings = settings;

    Reset();
}

void RenderPassVK::Reset()
{
    ITT_FUNCTION_TASK();

    uint32_t color_attach_index = 0;
    for(ColorAttachment& color_attach : m_settings.color_attachments)
    {
        if (color_attach.wp_texture.expired())
        {
            throw std::invalid_argument("Can not use color attachment without texture.");
        }

        TextureVK& color_texture = static_cast<TextureVK&>(*color_attach.wp_texture.lock());
        if (color_texture.GetSettings().type == Texture::Type::FrameBuffer)
        {
            color_texture.UpdateFrameBuffer();
        }
        
        color_attach_index++;
    }
    
    if (!m_settings.depth_attachment.wp_texture.expired())
    {
        const TextureVK& depth_texture = static_cast<const TextureVK&>(*m_settings.depth_attachment.wp_texture.lock());
    }
    
    if (!m_settings.stencil_attachment.wp_texture.expired())
    {
        const TextureVK& stencil_texture = static_cast<const TextureVK&>(*m_settings.stencil_attachment.wp_texture.lock());
    }
}

ContextVK& RenderPassVK::GetContextVK() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<class ContextVK&>(m_context);
}

} // namespace Methane::Graphics
