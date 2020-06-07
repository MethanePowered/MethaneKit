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

FILE: Methane/Graphics/Badge.h
Badge rendering primitive displaying fixed texture in specific corner of the screen.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ScreenQuad.h>

#include <memory>
#include <optional>

namespace Methane::Graphics
{

class ImageLoader;

class Badge : public ScreenQuad
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
        // Default settings of the Methane Logo badge
        FrameSize   size         = { 96u, 128u };
        FrameCorner corner       = FrameCorner::TopRight;
        Point2i     margins      = { 16, 16 };
        Color4f     blend_color  = Color4f(1.f, 1.f, 1.f, 1.f);
        TextureMode texture_mode = TextureMode::RgbaFloat;
    };

    explicit Badge(RenderContext& context);
    Badge(RenderContext& context, Settings settings);
    Badge(RenderContext& context, Ptr<Texture> sp_texture, Settings settings);

    void FrameResize(const FrameSize& frame_size, std::optional<FrameSize> badge_size = {}, std::optional<Point2i> margins = {});
    void SetCorner(FrameCorner frame_corner);
    void SetMargins(Point2i& margins);
    void SetSize(const FrameSize& size);

private:
    FrameRect GetBadgeRectInFrame(const FrameSize& frame_size);
    static FrameRect GetBadgeRectInFrame(const FrameSize& frame_size, const Settings& settings);

    Settings       m_settings;
    RenderContext& m_context;
};

} // namespace Methane::Graphics
