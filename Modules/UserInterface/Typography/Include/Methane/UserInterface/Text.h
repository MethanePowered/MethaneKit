/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/UserInterface/Text.h
Methane text rendering primitive.

******************************************************************************/

#pragma once

#include <Methane/UserInterface/Types.hpp>
#include <Methane/Graphics/Color.hpp>
#include <Methane/Data/Receiver.hpp>
#include <Methane/Data/Types.h>
#include <Methane/Pimpl.h>

#include <string_view>

namespace Methane::Graphics::Rhi
{
class RenderContext;
class RenderPattern;
class RenderCommandList;
class CommandListDebugGroup;
}

namespace Methane::UserInterface
{

enum class TextWrap : uint32_t
{
    None = 0U,
    Anywhere,
    Word
};

enum class TextHorizontalAlignment : uint32_t
{
    Left = 0U,
    Right,
    Center,
    Justify
};

enum class TextVerticalAlignment : uint32_t
{
    Top = 0U,
    Bottom,
    Center
};

struct TextLayout
{
    TextWrap                wrap                 = TextWrap::Anywhere;
    TextHorizontalAlignment horizontal_alignment = TextHorizontalAlignment::Left;
    TextVerticalAlignment   vertical_alignment   = TextVerticalAlignment::Top;

    [[nodiscard]] friend bool operator==(const TextLayout& left, const TextLayout& right) noexcept = default;
};

template<typename StringType>
struct TextSettings // NOSONAR
{
    std::string name;
    StringType  text;
    UnitRect    rect;
    TextLayout  layout;
    Color4F     color { 1.F, 1.F, 1.F, 1.F };
    bool        incremental_update = true;
    bool        adjust_vertical_content_offset = true;

    // Minimize number of vertex/index buffer re-allocations on dynamic text updates by reserving additional size with multiplication of required size
    Data::Size  mesh_buffers_reservation_multiplier = 2U;

    // Text render state object name for using as a key in graphics object cache
    // NOTE: State name should be different in case of render state incompatibility between Text objects
    std::string state_name = "Screen Text Render State";

    TextSettings& SetName(std::string_view new_name) noexcept                                         { name = new_name; return *this; }
    TextSettings& SetText(const StringType& new_text) noexcept                                        { text = new_text; return *this; }
    TextSettings& SetRect(const UnitRect& new_rect) noexcept                                          { rect = new_rect; return *this; }
    TextSettings& SetLayout(const TextLayout& new_layout) noexcept                                    { layout = new_layout; return *this; }
    TextSettings& SetColor(const Color4F& new_color) noexcept                                         { color = new_color; return *this; }
    TextSettings& SetIncrementalUpdate(bool new_incremental_update) noexcept                          { incremental_update = new_incremental_update; return *this; }
    TextSettings& SetAdjustVerticalContentOffset(bool new_adjust_offset) noexcept                     { adjust_vertical_content_offset = new_adjust_offset; return *this; }
    TextSettings& SetMeshBuffersReservationMultiplier(Data::Size new_reservation_multiplier) noexcept { mesh_buffers_reservation_multiplier = new_reservation_multiplier; return *this; }
    TextSettings& SetStateName(std::string_view new_state_name) noexcept                              { state_name = new_state_name; return *this; }
};

struct ITextCallback
{
    virtual void OnTextFrameRectChanged(const UnitRect& frame_rect) = 0;

    virtual ~ITextCallback() = default;
};

class Context;
class Font;

namespace rhi = Methane::Graphics::Rhi;

class Text // NOSONAR - manual copy, move constructors and assignment operators
{
public:
    using Wrap                = TextWrap;
    using HorizontalAlignment = TextHorizontalAlignment;
    using VerticalAlignment   = TextVerticalAlignment;
    using Layout              = TextLayout;

    template<typename StringType>
    using Settings      = TextSettings<StringType>;
    using SettingsUtf8  = Settings<std::string>;
    using SettingsUtf32 = Settings<std::u32string>;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE_NO_INLINE(Text);

    Text(Context& ui_context, const rhi::RenderPattern& render_pattern, const Font& font, const SettingsUtf8& settings);
    Text(Context& ui_context, const Font& font, const SettingsUtf8& settings);
    Text(Context& ui_context, const rhi::RenderPattern& render_pattern, const Font& font, const SettingsUtf32& settings);
    Text(Context& ui_context, const Font& font, const SettingsUtf32& settings);

    bool IsInitialized() const noexcept { return static_cast<bool>(m_impl_ptr); }

    void Connect(Data::Receiver<ITextCallback>& callback) const;
    void Disconnect(Data::Receiver<ITextCallback>& callback) const;

    [[nodiscard]] const UnitRect&       GetFrameRect() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] const SettingsUtf32&  GetSettings() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] const std::u32string& GetTextUtf32() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] std::string           GetTextUtf8() const;

    void SetText(std::string_view text) const;
    void SetText(std::u32string_view text) const;
    void SetTextInScreenRect(std::string_view text, const UnitRect& ui_rect) const;
    void SetTextInScreenRect(std::u32string_view text, const UnitRect& ui_rect) const;
    void SetColor(const gfx::Color4F& color) const;
    void SetLayout(const Layout& layout) const;
    void SetWrap(Wrap wrap) const;
    void SetHorizontalAlignment(HorizontalAlignment alignment) const;
    void SetVerticalAlignment(VerticalAlignment alignment) const;
    void SetIncrementalUpdate(bool incremental_update) const META_PIMPL_NOEXCEPT;
    bool SetFrameRect(const UnitRect& ui_rect) const;

    void Update(const gfx::FrameSize& frame_size) const;
    void Draw(const rhi::RenderCommandList& cmd_list, const rhi::CommandListDebugGroup* debug_group_ptr = nullptr) const;

private:
    class Impl;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics
