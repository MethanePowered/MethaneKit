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

FILE: Methane/Graphics/ScreenQuad.cpp
Logo badge rendering primitive.

******************************************************************************/

#include <Methane/Graphics/LogoBadge.h>

#include <Methane/Graphics/ImageLoader.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

#include <cmath>

namespace Methane::Graphics
{

static LogoBadge::Settings ScaleBadgeSize(LogoBadge::Settings settings, float scale_factor)
{
    settings.size.width  = static_cast<uint32_t>(std::round(scale_factor * settings.size.width));
    settings.size.height = static_cast<uint32_t>(std::round(scale_factor * settings.size.height));
    settings.margins     = static_cast<uint32_t>(std::round(scale_factor * settings.margins));
    return settings;
}
    
LogoBadge::LogoBadge(RenderContext& context, Settings settings)
    : LogoBadge(context,
                ImageLoader(Data::TextureProvider::Get()).LoadImageToTexture2D(context, "Logo/MethaneLogoNameWatermark.png", true),
                ScaleBadgeSize(settings, context.GetContentScalingFactor()))
{
    ITT_FUNCTION_TASK();
}

LogoBadge::LogoBadge(RenderContext& context, Ptr<Texture> sp_texture, Settings settings)
    : ScreenQuad(context, std::move(sp_texture),
                 ScreenQuad::Settings
                 {
                     "Logo Badge",
                     GetBadgeRectInFrame(context.GetSettings().frame_size, settings.corner, settings.size, settings.margins),
                     true,
                     Color4f(1.f, 1.f, 1.f, settings.opacity)
                 })
    , m_settings(std::move(settings))
{
    ITT_FUNCTION_TASK();
}

void LogoBadge::Resize(const FrameSize& frame_size)
{
    ITT_FUNCTION_TASK();
    SetScreenRect(GetBadgeRectInFrame(frame_size, m_settings.corner, m_settings.size, m_settings.margins));
}

FrameRect LogoBadge::GetBadgeRectInFrame(const FrameSize& frame_size, FrameCorner frame_corner,
                                         const FrameSize& badge_size, uint32_t badge_margins)
{
    ITT_FUNCTION_TASK();

    switch(frame_corner)
    {
    case FrameCorner::TopLeft:
        return FrameRect{ FrameRect::Point(badge_margins, badge_margins), badge_size };

    case FrameCorner::TopRight:
        return FrameRect{ FrameRect::Point(frame_size.width - badge_size.width - badge_margins, badge_margins), badge_size };

    case FrameCorner::BottomLeft:
        return FrameRect{ FrameRect::Point(badge_margins, frame_size.height - badge_size.height - badge_margins), badge_size };

    case FrameCorner::BottomRight:
        return FrameRect{
            FrameRect::Point(
                frame_size.width - badge_size.width - badge_margins,
                frame_size.height - badge_size.height - badge_margins),
            badge_size
        };
    }

    return FrameRect();
}

} // namespace Methane::Graphics
