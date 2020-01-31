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

FILE: Methane/Graphics/LogoBadge.h
Logo badge rendering primitive.

******************************************************************************/

#pragma once

#include "ScreenQuad.h"

#include <memory>

namespace Methane::Graphics
{

class ImageLoader;

class LogoBadge : public ScreenQuad
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
        FrameSize   size;
        FrameCorner corner;
        uint32_t    margins;
        float       opacity;
        
        Settings()
            : size(96u, 128u)
            , corner(FrameCorner::TopRight)
            , margins(16u)
            , opacity(0.15f)
        { }
    };

    LogoBadge(RenderContext& context, Settings settings = Settings());
    LogoBadge(RenderContext& context, Ptr<Texture> sp_texture, Settings settings = Settings());

    void Resize(const FrameSize& frame_size);

private:
    static FrameRect GetBadgeRectInFrame(const FrameSize& frame_size, FrameCorner frame_corner, 
                                         const FrameSize& badge_size, uint32_t badge_margins);

    const Settings m_settings;
};

} // namespace Methane::Graphics
