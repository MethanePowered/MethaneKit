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

FILE: Methane/UserInterface/Badge.h
Badge widget displaying texture in specific corner of the screen.

******************************************************************************/

#pragma once

#include <Methane/UserInterface/Item.h>

#include <Methane/Graphics/ScreenQuad.h>

#include <string_view>

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
        TopLeft = 0U,
        TopRight,
        BottomRight,
        BottomLeft
    };

    struct Settings
    {
        std::string       name         = "Badge";
        FrameCorner       corner       = FrameCorner::TopRight;
        UnitSize          size         { Units::Dots, 96U, 128U };
        UnitSize          margins      { Units::Dots, 16, 16 };
        Color4F           blend_color  { 1.F, 1.F, 1.F, 1.F };
        TextureMode       texture_mode = TextureMode::RgbaFloat;

        Settings& SetName(std::string_view new_name) noexcept;
        Settings& SetSize(const UnitSize& new_size) noexcept;
        Settings& SetCorner(FrameCorner new_corner) noexcept;
        Settings& SetMargings(const UnitSize& new_margins) noexcept;
        Settings& SetBlendColor(const Color4F& new_blend_color) noexcept;
        Settings& SetTextureMode(TextureMode new_texture_mode) noexcept;
    };

    Badge(Context& ui_context, Data::Provider& data_provider, const std::string& image_path, const Settings& settings);
    Badge(Context& ui_context, const Ptr<gfx::Texture>& texture_ptr, const Settings& settings);

    void FrameResize(const UnitSize& frame_size, Opt<UnitSize> badge_size = {}, Opt<UnitSize> margins = {});
    void SetCorner(FrameCorner frame_corner);
    void SetMargins(const UnitSize& margins);

private:
    // Item overrides
    bool SetRect(const UnitRect& ui_rect) override;
    using Item::SetOrigin;

    UnitRect GetBadgeRectInFrame() { return GetBadgeRectInFrame(GetUIContext(), m_frame_size, m_settings); }
    static UnitRect GetBadgeRectInFrame(const Context& ui_context, const UnitSize& frame_size, const Settings& settings);
    static UnitRect GetBadgeRectInFrame(const UnitSize& frame_size, const UnitSize& badge_size,
                                        const UnitSize& badge_margins, Badge::FrameCorner frame_corner);

    Settings m_settings;
    UnitSize m_frame_size;
};

} // namespace Methane::UserInterface
