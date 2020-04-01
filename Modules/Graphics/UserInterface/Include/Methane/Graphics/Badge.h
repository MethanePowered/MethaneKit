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
        FrameSize   size        = { 96u, 128u };
        FrameCorner corner      = FrameCorner::TopRight;
        uint32_t    margins     = 16u;
        float       opacity     = 0.15f;
        TextureMode texure_mode = TextureMode::RgbaFloat;
    };

    explicit Badge(RenderContext& context);
    Badge(RenderContext& context, Settings settings);
    Badge(RenderContext& context, Ptr<Texture> sp_texture, Settings settings);

    void Resize(const FrameSize& frame_size);

private:
    static FrameRect GetBadgeRectInFrame(const FrameSize& frame_size, FrameCorner frame_corner, 
                                         const FrameSize& badge_size, uint32_t badge_margins);

    const Settings m_settings;
};

} // namespace Methane::Graphics
