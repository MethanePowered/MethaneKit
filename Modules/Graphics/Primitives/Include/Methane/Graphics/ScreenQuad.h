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

FILE: Methane/Graphics/ScreenQuad.h
ScreenQuad rendering primitive.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Rect.hpp>
#include <Methane/Graphics/Color.hpp>
#include <Methane/Memory.hpp>
#include <Methane/Pimpl.h>

#include <string>

namespace Methane::Graphics::Rhi
{

class Texture;
class CommandQueue;
class RenderPattern;
class RenderCommandList;
class CommandListDebugGroup;

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics
{

class ScreenQuad
{
public:
    enum class TextureMode : uint32_t
    {
        Disabled = 0U,
        RgbaFloat,
        RFloatToAlpha,
    };

    struct Settings
    {
        const std::string name;
        FrameRect         screen_rect;
        bool              alpha_blending_enabled = false;
        Color4F           blend_color            { 1.F, 1.F, 1.F, 1.F };
        TextureMode       texture_mode           = TextureMode::RgbaFloat;
    };

    ScreenQuad() = default;
    ScreenQuad(const Rhi::CommandQueue& render_cmd_queue, const Rhi::RenderPattern& render_pattern, const Settings& settings);
    ScreenQuad(const Rhi::CommandQueue& render_cmd_queue, const Rhi::RenderPattern& render_pattern, const Rhi::Texture& texture, const Settings& settings);
    virtual ~ScreenQuad() = default;

    void SetBlendColor(const Color4F& blend_color);
    void SetScreenRect(const FrameRect& screen_rect, const FrameSize& render_attachment_size);
    void SetAlphaBlendingEnabled(bool alpha_blending_enabled);
    void SetTexture(Rhi::Texture texture);

    [[nodiscard]] const Settings& GetQuadSettings() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] const Rhi::Texture& GetTexture() const META_PIMPL_NOEXCEPT;

    virtual void Draw(const Rhi::RenderCommandList& cmd_list, const Rhi::CommandListDebugGroup* debug_group_ptr = nullptr) const;

    bool IsInitialized() const noexcept { return static_cast<bool>(m_impl_ptr); }

private:
    class Impl;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics
