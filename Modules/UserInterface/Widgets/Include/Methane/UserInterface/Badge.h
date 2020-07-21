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

FILE: Methane/UserInterface/Badge.h
Badge widget displaying texture in specific corner of the screen.

******************************************************************************/

#pragma once

#include <Methane/UserInterface/Item.h>

#include <Methane/Graphics/ScreenQuad.h>

namespace Methane::Graphics
{
class ImageLoader;
}

namespace Methane::UserInterface
{

class Badge
    : public Item
    , public gfx::ScreenQuad
{
public:
    enum class FrameCorner : uint32_t
    {
        TopLeft = 0u,
        TopRight,
        BottomRight,
        BottomLeft
    };

    struct Settings
    {
        const std::string name;
        UnitSize          size               { 96u, 128u, Units::Dots };
        FrameCorner       corner             = FrameCorner::TopRight;
        UnitPoint         margins            { 16, 16, Units::Dots };
        Color4f           blend_color        { 1.f, 1.f, 1.f, 1.f };
        TextureMode       texture_mode       = TextureMode::Constant;
        TextureColorMode  texture_color_mode = TextureColorMode::RgbaFloat;
    };

    Badge(Context& ui_context, Data::Provider& data_provider, const std::string& image_path, Settings settings);
    Badge(Context& ui_context, Ptr<gfx::Texture> sp_texture, Settings settings);

    void FrameResize(const UnitSize& frame_size, std::optional<UnitSize> badge_size = {}, std::optional<UnitPoint> margins = {});
    void SetCorner(FrameCorner frame_corner);
    void SetMargins(UnitPoint& margins);

private:
    // Item overrides
    bool SetRect(const UnitRect& ui_rect) override;
    using Item::SetOrigin;

    UnitRect GetBadgeRectInFrame(const UnitSize& frame_size) { return GetBadgeRectInFrame(GetUIContext(), frame_size, m_settings); }
    static UnitRect GetBadgeRectInFrame(Context& ui_context, const UnitSize& frame_size, const Settings& settings);
    static UnitRect GetBadgeRectInFrame(const UnitSize& frame_size, const UnitSize& badge_size,
                                        const UnitPoint& badge_margins, Badge::FrameCorner frame_corner);

    Settings m_settings;
};

} // namespace Methane::UserInterface