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

FILE: Methane/Graphics/Vulkan/RenderPassVK.mm
Vulkan implementation of the render pass interface.

******************************************************************************/

#include "RenderPassVK.h"
#include "ContextVK.h"
#include "TextureVK.h"

#include <Methane/Graphics/RenderContextBase.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

Ptr<RenderPass> RenderPass::Create(RenderContext& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderPassVK>(dynamic_cast<RenderContextBase&>(context), settings);
}

RenderPassVK::RenderPassVK(RenderContextBase& context, const Settings& settings)
    : RenderPassBase(context, settings)
{
    META_FUNCTION_TASK();
    Reset();
}

bool RenderPassVK::Update(const Settings& settings)
{
    META_FUNCTION_TASK();
    const bool settings_changed = RenderPassBase::Update(settings);
    Reset();
    return settings_changed;
}

void RenderPassVK::Reset()
{
    META_FUNCTION_TASK();
    uint32_t color_attach_index = 0;
    for(const ColorAttachment& color_attach : GetSettings().color_attachments)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(color_attach.texture_location.IsInitialized(), "can not use color attachment without texture");
        if (auto& color_texture = static_cast<TextureVK&>(color_attach.texture_location.GetTexture());
            color_texture.GetSettings().type == Texture::Type::FrameBuffer)
        {
            color_texture.UpdateFrameBuffer();
        }
        
        color_attach_index++;
    }
}

IContextVK& RenderPassVK::GetContextVK() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<IContextVK&>(GetRenderContext());
}

} // namespace Methane::Graphics
