/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

#include <Methane/Graphics/RenderContextBase.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<RenderPass> RenderPass::Create(RenderContext& context, const Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<RenderPassVK>(dynamic_cast<RenderContextBase&>(context), settings);
}

RenderPassVK::RenderPassVK(RenderContextBase& context, const Settings& settings)
    : RenderPassBase(context, settings)
{
    ITT_FUNCTION_TASK();
    Reset();
}

void RenderPassVK::Update(const Settings& settings)
{
    ITT_FUNCTION_TASK();
    RenderPassBase::Update(settings);
    Reset();
}

void RenderPassVK::Reset()
{
    ITT_FUNCTION_TASK();

    uint32_t color_attach_index = 0;
    for(const ColorAttachment& color_attach : GetSettings().color_attachments)
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
}

IContextVK& RenderPassVK::GetContextVK() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<IContextVK&>(GetRenderContext());
}

} // namespace Methane::Graphics
