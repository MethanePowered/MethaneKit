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
Badge rendering primitive displaying fixed texture in specific corner of the screen.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ScreenQuad.h>

#include <memory>
#include <optional>

namespace Methane::Graphics
{
class ImageLoader;
}

namespace Methane::UserInterface
{

namespace gfx = Methane::Graphics;

class Badge : public gfx::ScreenQuad
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
        gfx::FrameSize    size               = { 96u, 128u };
        FrameCorner       corner             = FrameCorner::TopRight;
        gfx::Point2i      margins            = { 16, 16 };
        gfx::Color4f      blend_color        = gfx::Color4f(1.f, 1.f, 1.f, 1.f);
        TextureMode       texture_mode       = TextureMode::Constant;
        TextureColorMode  texture_color_mode = TextureColorMode::RgbaFloat;
    };

    Badge(gfx::RenderContext& context, Data::Provider& data_provider, const std::string& image_path, Settings settings);
    Badge(gfx::RenderContext& context, Ptr<gfx::Texture> sp_texture, Settings settings);

    void FrameResize(const gfx::FrameSize& frame_size, std::optional<gfx::FrameSize> badge_size = {}, std::optional<gfx::Point2i> margins = {});
    void SetCorner(FrameCorner frame_corner);
    void SetMargins(gfx::Point2i& margins);
    void SetSize(const gfx::FrameSize& size);

private:
    gfx::FrameRect GetBadgeRectInFrame(const gfx::FrameSize& frame_size);
    static gfx::FrameRect GetBadgeRectInFrame(const gfx::FrameSize& frame_size, const Settings& settings);

    Settings            m_settings;
    gfx::RenderContext& m_context;
};

} // namespace Methane::UserInterface
