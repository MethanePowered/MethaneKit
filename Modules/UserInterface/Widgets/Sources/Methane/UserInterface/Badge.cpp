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

FILE: Methane/UserInterface/Badge.cpp
Badge widget displaying texture in specific corner of the screen.

******************************************************************************/

#include <Methane/UserInterface/Badge.h>
#include <Methane/UserInterface/Context.h>

#include <Methane/Graphics/ImageLoader.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

#include <cmath>
#include <fmt/format.h>

namespace Methane::UserInterface
{

Badge::Settings& Badge::Settings::SetName(std::string_view new_name) noexcept
{
    META_FUNCTION_TASK();
    name = new_name;
    return *this;
}

Badge::Settings& Badge::Settings::SetSize(const UnitSize& new_size) noexcept
{
    META_FUNCTION_TASK();
    size = new_size;
    return *this;
}

Badge::Settings& Badge::Settings::SetCorner(FrameCorner new_corner) noexcept
{
    META_FUNCTION_TASK();
    corner = new_corner;
    return *this;
}

Badge::Settings& Badge::Settings::SetMargings(const UnitSize& new_margins) noexcept
{
    META_FUNCTION_TASK();
    margins = new_margins;
    return *this;
}

Badge::Settings& Badge::Settings::SetBlendColor(const Color4F& new_blend_color) noexcept
{
    META_FUNCTION_TASK();
    blend_color = new_blend_color;
    return *this;
}

Badge::Settings& Badge::Settings::SetTextureMode(TextureMode new_texture_mode) noexcept
{
    META_FUNCTION_TASK();
    texture_mode = new_texture_mode;
    return *this;
}

Badge::Badge(Context& ui_context, Data::IProvider& data_provider, const std::string& image_path, const Settings& settings)
    : Badge(ui_context,
            gfx::ImageLoader(data_provider).LoadImageToTexture2D(ui_context.GetRenderCommandQueue(),
                                                                 image_path, gfx::ImageLoader::OptionMask{},
                                                                 fmt::format("{} Texture", settings.name)),
            settings)
{
}

Badge::Badge(Context& ui_context, const rhi::Texture& texture, const Settings& settings)
    : Item(ui_context, GetBadgeRectInFrame(ui_context, ui_context.GetFrameSizeIn<Units::Pixels>(), settings))
    , ScreenQuad(ui_context.GetRenderCommandQueue(), ui_context.GetRenderPattern(), texture,
        ScreenQuad::Settings
        {
            settings.name,
            GetRectInPixels().AsBase(),
            true, // alpha_blending_enabled
            settings.blend_color,
            settings.texture_mode,
        }
    )
    , m_settings(settings)
    , m_frame_size(ui_context.GetFrameSizeIn<Units::Pixels>())
{
    META_FUNCTION_TASK();
}

void Badge::FrameResize(const UnitSize& frame_size, Opt<UnitSize> badge_size, Opt<UnitSize> margins)
{
    META_FUNCTION_TASK();
    
    m_frame_size = frame_size;
    
    if (badge_size)
    {
        m_settings.size = *badge_size;
    }
    if (margins)
    {
        m_settings.margins = *margins;
    }
    SetRect(GetBadgeRectInFrame());
}

void Badge::SetCorner(FrameCorner frame_corner)
{
    META_FUNCTION_TASK();
    if (m_settings.corner == frame_corner)
        return;

    m_settings.corner = frame_corner;

    SetRect(GetBadgeRectInFrame());
}

void Badge::SetMargins(const UnitSize& margins)
{
    META_FUNCTION_TASK();
    if (m_settings.margins == margins)
        return;

    m_settings.margins = margins;

    SetRect(GetBadgeRectInFrame());
}

bool Badge::SetRect(const UnitRect& ui_rect)
{
    META_FUNCTION_TASK();
    if (!Item::SetRect(ui_rect))
        return false;

    gfx::ScreenQuad::SetScreenRect(Item::GetRectInPixels(), m_frame_size);
    return true;
}

UnitRect Badge::GetBadgeRectInFrame(const Context& ui_context, const UnitSize& frame_size, const Settings& settings)
{
    return GetBadgeRectInFrame(frame_size,
                               ui_context.ConvertToUnits(settings.size,    frame_size.GetUnits()),
                               ui_context.ConvertToUnits(settings.margins, frame_size.GetUnits()),
                               settings.corner);
}

UnitRect Badge::GetBadgeRectInFrame(const UnitSize& frame_size, const UnitSize& badge_size,
                                    const UnitSize& badge_margins, Badge::FrameCorner frame_corner)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_DESCR(frame_size.GetUnits(), frame_size.GetUnits() == badge_size.GetUnits() && badge_size.GetUnits() == badge_margins.GetUnits(),
                         "frame size, badge size and margin units must be equal");

    switch(frame_corner)
    {
    case FrameCorner::TopLeft:
        return UnitRect(frame_size.GetUnits(), gfx::FramePoint(badge_margins.GetWidth(), badge_margins.GetHeight()), badge_size);

    case FrameCorner::TopRight:
        return UnitRect(frame_size.GetUnits(), gfx::FramePoint(frame_size.GetWidth() - badge_size.GetWidth() - badge_margins.GetWidth(), badge_margins.GetHeight()), badge_size);

    case FrameCorner::BottomLeft:
        return UnitRect(frame_size.GetUnits(), gfx::FramePoint(badge_margins.GetWidth(), frame_size.GetHeight() - badge_size.GetHeight() - badge_margins.GetHeight()), badge_size);

    case FrameCorner::BottomRight:
        return UnitRect(frame_size.GetUnits(), static_cast<gfx::FramePoint>(frame_size - badge_size - badge_margins), badge_size);

    default:
        META_UNEXPECTED_ARG_RETURN(frame_corner, UnitRect());
    }
}

} // namespace Methane::UserInterface
