# Typography Tutorial

| <pre><b>Windows (DirectX 12)       </pre></b>                    | <pre><b>Linux (Vulkan)             </pre></b>               | <pre><b>MacOS (Metal)              </pre></b>              | <pre><b>iOS (Metal)</pre></b>                              |
|------------------------------------------------------------------|-------------------------------------------------------------|------------------------------------------------------------|------------------------------------------------------------|
| ![Typography on Windows](Screenshots/TypographyWinDirectX12.jpg) | ![Typography on Linux](Screenshots/TypographyLinVulkan.jpg) | ![Typography on MacOS](Screenshots/TypographyMacMetal.jpg) | ![Typography on MacOS](Screenshots/TypographyIOSMetal.jpg) |

This tutorial demonstrates animated text rendering with dynamic font atlas updates using Methane UI. Three colored text blocks 
are animated with continuous character typing. Each text block is rendered as a single mesh displaying character glyphs from the 
font atlas texture to screen rectangles, which are generated on the CPU using the Freetype 2.0 library.

The font atlas texture can be updated dynamically by adding new character glyphs on demand, as the user types text, including 
any non-ASCII character sets. Text character layout and mesh generation are done on the CPU using Methane implementation, 
without using third-party libraries, and support horizontal and vertical text alignment in rectangular areas with wrapping by 
characters and words. Right-to-left and Arabic language character layout is not supported yet.

## Application Controls

Keyboard actions are enabled with [TypographyAppController](TypographyAppController.h) class
derived from [Platform::Input::Keyboard::ActionControllerBase](/Modules/Platform/Input/ActionControllers/Include/Methane/Platform/Input/KeyboardActionControllerBase.hpp):

| Typography App Action                                           | Keyboard Shortcut |
|-----------------------------------------------------------------|-------------------|
| Switch Text Wrap Mode (None, Anywhere, Word)                    | `W`               |
| Switch Text Horizontal Alignment (Left, Right, Center, Justify) | `H`               |
| Switch Text Vertical Alignment (Top, Bottom, Center)            | `V`               |
| Switch Incremental Text Update                                  | `U`               |
| Switch Typing Direction (Forward, Backward)                     | `D`               |
| Speedup Typing                                                  | `+`               |
| Slowdown Typing                                                 | `-`               |

Common keyboard controls are enabled by the `Platform`, `Graphics` and `UserInterface` application controllers:
- [Methane::Platform::AppController](/Modules/Platform/App/README.md#platform-application-controller)
- [Methane::Graphics::AppController, AppContextController](/Modules/Graphics/App/README.md#graphics-application-controllers)
- [Methane::UserInterface::AppController](/Modules/UserInterface/App/README.md#user-interface-application-controllers)

## Application and Frame Class Definitions

`TypographyApp` class is declared in header file `TypographyApp.h`, and is derived from
[UserInterface::App](../../Modules/UserInterface/App) base class, same as in [previous tutorial](../03-ShadowCube).
Base application class `UserInterface::App<TypographyFrame>` uses frame structure `TypographyFrame`, which defines only
the render command list and execution command list set that wraps this command list.

`TypographyApp` class defines a settings structure `TypographyApp::Settings`, a getter for the current settings 
`TypographyApp::GetSettings`, and individual setters for each application setting:
- `TypographyApp::SetTextLayout` - allows changing word wrapping mode, horizontal and vertical text layouts for all text blocks;
- `TypographyApp::SetForwardTypingDirection` - changes text typing animation: appending new characters if `true`, or backspace 
  deleting if `false`;
- `TypographyApp::SetTextUpdateInterval` - changes text typing time interval in milliseconds;
- `TypographyApp::SetIncrementalTextUpdate` - enables or disables incremental updating of text block mesh buffers.

Application setting getters can be changed by the user at runtime with keyboard shortcuts, handled in
[TypographyAppController.h](TypographyAppController.h).

`TypographyApp` class contains the following private members:
- `gui::Font` objects, each for a unique font, size, and color;
- `gui::TextItem` objects, each for one text block;
- `gui::Badge` objects for rendering font atlas textures on screen;
- `std::vector<size_t>` displayed lengths of text in each text block, incremented with animation;
- `Timer::TimeDuration` holds the duration of the last text block update to be displayed on screen.

```cpp
#pragma once

#include <Methane/Kit.h>
#include <Methane/Data/Receiver.hpp>

namespace Methane::Tutorials
{

namespace gfx = Methane::Graphics;
namespace rhi = Methane::Graphics::Rhi;
namespace gui = Methane::UserInterface;

struct TypographyFrame final : gfx::AppFrame
{
    rhi::RenderCommandList render_cmd_list;
    rhi::CommandListSet    execute_cmd_list_set;

    using gfx::AppFrame::AppFrame;
};

using UserInterfaceApp = UserInterface::App<TypographyFrame>;

class TypographyApp final
    : public UserInterfaceApp
    , private Data::Receiver<gui::IFontLibraryCallback>
    , private Data::Receiver<gui::IFontCallback>
{
public:
    struct Settings
    {
        gui::Text::Layout text_layout { gui::Text::Wrap::Word, gui::Text::HorizontalAlignment::Center, gui::Text::VerticalAlignment::Top };
        bool              is_incremental_text_update  = true;
        bool              is_forward_typing_direction = true;
        double            typing_update_interval_sec  = 0.03;
    };

    TypographyApp();
    ~TypographyApp() override;

    // GraphicsApp overrides
    ...

    // UserInterface::App overrides
    std::string GetParametersString() override;

    // Settings accessors
    const Settings& GetSettings() const noexcept { return m_settings; }
    void SetTextLayout(const gui::Text::Layout& text_layout);
    void SetForwardTypingDirection(bool is_forward_typing_direction);
    void SetTextUpdateInterval(double text_update_interval_sec);
    void SetIncrementalTextUpdate(bool is_incremental_text_update);

private:
    ...

    Settings               m_settings;
    gui::FontContext       m_font_context;
    std::vector<gui::Font> m_fonts;
    Ptrs<gui::TextItem>    m_texts;
    Ptrs<gui::Badge>       m_font_atlas_badges;
    std::vector<size_t>    m_displayed_text_lengths;
    double                 m_text_update_elapsed_sec = 0.0;
    Timer::TimeDuration    m_text_update_duration;
};

} // namespace Methane::Tutorials
```

## Graphics Resources Initialization

Fonts and text blocks are initialized in the `TypographyApp::Init()` method in a `for` loop for each block index. Text blocks 
are positioned one below another using the `vertical_text_pos_in_dots` variable, which holds the vertical position in 
DPI-independent Dot units.

```cpp
    const gfx::FrameSize frame_size_in_dots = GetFrameSizeInDots();
    const uint32_t frame_width_without_margins = frame_size_in_dots.GetWidth() - 2 * g_margin_size_in_dots;
    int32_t vertical_text_pos_in_dots = g_top_text_pos_in_dots;

    for(size_t block_index = 0; block_index < g_text_blocks_count; ++block_index)
    {
        const FontSettings& font_settings = g_font_settings[block_index];
        const size_t displayed_text_length = m_displayed_text_lengths[block_index];
        const std::u32string displayed_text_block = g_text_blocks[block_index].substr(0, displayed_text_length);

        // Add font to library
        m_fonts.push_back( ... );

        // Add text element
        m_texts.push_back( ... );

        vertical_text_pos_in_dots = m_texts.back()->GetRectInDots().GetBottom() + g_margin_size_in_dots;
    }
```

Font objects are used to create and render text blocks on screen. Font objects are loaded from the `Data::FontProvider` 
singleton with specified font settings and are added to the fonts library. `Data::FontProvider` loads `TTF` fonts from 
application resources. Font settings include:
- Font description:
  - Unique name for registration in the fonts library
  - File path loaded with the data provider
  - Size in points
- Font resolution DPI
- Initial alphabet to be rendered and added to the font atlas (it is also dynamically extended on demand)

`UserInterface::FontContext` is a helper class that holds references to `FontLibrary` and `Data::IProvider` to facilitate 
font creation and queries via the `FontContext::GetFont` method.

```cpp
        // Add font to library
        m_fonts.push_back(
            m_font_context.GetFont(
                gui::Font::Settings
                {
                    .description    = font_settings.desc,
                    .resolution_dpi = GetUIContext().GetFontResolutionDpi(),
                    .characters     = gui::Font::GetAlphabetFromText(displayed_text_block)
                }
            )
        );
```

Each text block object is created using the `UserInterface::Context` object acquired with the `UserInterface::App::GetUIContext()`
method, the font object which was created previously, and text settings:
- Font name registered in the fonts library
- Text string in UTF8 or UTF32 format
- Rectangle area in Dots or Pixels on the screen to fit the text in
- Layout describing how the text will fit into the rectangular area
  - Wrapping mode (No wrapping, Wrap anywhere, Wrap on word boundaries)
  - Horizontal alignment (Left, Right, Center)
  - Vertical alignment (Top, Bottom, Center)
- Color of the text block
- Incremental text mesh update flag

```cpp
        // Add text element
        m_texts.push_back(
            std::make_shared<gui::TextItem>(GetUIContext(), m_fonts.back(),
                gui::Text::SettingsUtf32
                {
                    .name = font_settings.desc.name,
                    .text = displayed_text_block,
                    .rect = gui::UnitRect
                    {
                        gui::Units::Dots,
                        gfx::Point2I { g_margin_size_in_dots, vertical_text_pos_in_dots },
                        gfx::FrameSize { frame_width_without_margins, 0U /* calculated height */ }
                    },
                    .layout = m_settings.text_layout,
                    .color  = gfx::Color4F(font_settings.color, 1.F),
                    .incremental_update = m_settings.is_incremental_text_update
                }
            )
        );
```

Font objects hold font atlas textures, which are displayed on screen in this tutorial using `UserInterface::Badge` objects.
These badges are created in the `TypographyApp::UpdateFontAtlasBadges()` method:

```cpp
oid TypographyApp::UpdateFontAtlasBadges()
{
    const std::vector<gui::Font> fonts = m_font_context.GetFontLibrary().GetFonts();
    const rhi::RenderContext&  context = GetRenderContext();

    // Remove obsolete font atlas badges
    ...

    // Add new font atlas badges
    for(const gui::Font& font : fonts)
    {
        const rhi::Texture& font_atlas_texture = font.GetAtlasTexture(context);
        if (!font_atlas_texture.IsInitialized() ||
            std::any_of(m_font_atlas_badges.begin(), m_font_atlas_badges.end(),
                        [&font_atlas_texture](const Ptr<gui::Badge>& font_atlas_badge_ptr)
                        {
                            return font_atlas_badge_ptr->GetTexture() == font_atlas_texture;
                        }))
            continue;

        m_font_atlas_badges.emplace_back(CreateFontAtlasBadge(font, font_atlas_texture));
    }

    LayoutFontAtlasBadges(GetRenderContext().GetSettings().frame_size);
}
```

Font atlas badge bound to atlas texture is created with helper method `TypographyApp::CreateFontAtlasBadge`:

```cpp
Ptr<gui::Badge> TypographyApp::CreateFontAtlasBadge(const gui::Font& font, const rhi::Texture& atlas_texture)
{
    const auto font_color_by_name_it = g_font_color_by_name.find(font.GetSettings().description.name);
    const gui::Color3F& font_color = font_color_by_name_it != g_font_color_by_name.end()
                                   ? font_color_by_name_it->second : g_misc_font_color;

    return std::make_shared<gui::Badge>(
        GetUIContext(), atlas_texture,
        gui::Badge::Settings
        {
            .name         = font.GetSettings().description.name + " Font Atlas",
            .corner       = gui::Badge::FrameCorner::BottomLeft,
            .size         = gui::UnitSize(gui::Units::Pixels, static_cast<const gfx::FrameSize&>(atlas_texture.GetSettings().dimensions)),
            .margins      = gui::UnitSize(gui::Units::Dots, 16U, 16U),
            .blend_color  = gui::Color4F(font_color, 0.5F),
            .texture_mode = gui::Badge::TextureMode::RFloatToAlpha,
        }
    );
}
```

`TypographyApp::LayoutFontAtlasBadges` positions all created atlas badges in the bottom-left corner of the screen, 
arranging them one after another, starting with the largest textures and ending with the smallest.

## Frame Rendering Cycle

Animation function bound to time-animation in the constructor of the `TypographyApp` class is called automatically as a part 
of every render cycle, just before the `App::Update` function call. The `TypographyApp::Animate` function updates the text 
displayed in text blocks by adding or deleting characters and updates text block positions at equal time intervals.

```cpp
TypographyApp::TypographyApp()
{
    ...
    
    GetAnimations().emplace_back(std::make_shared<Data::TimeAnimation>(std::bind(&ShadowCubeApp::Animate, this, std::placeholders::_1, std::placeholders::_2)));
}

bool TypographyApp::Animate(double elapsed_seconds, double)
{
    if (elapsed_seconds - m_text_update_elapsed_sec < m_settings.typing_update_interval_sec)
        return true;

    m_text_update_elapsed_sec = elapsed_seconds;

    int32_t vertical_text_pos_in_dots = g_top_text_pos_in_dots;
    for(size_t block_index = 0; block_index < g_text_blocks_count; ++block_index)
    {
        AnimateTextBlock(block_index, vertical_text_pos_in_dots);
    }

    UpdateParametersText();
    return true;
}
```

Each text rendering object manages its own set of GPU resources for each frame in the swap-chain:
- Vertex and index buffers
- Shader uniforms buffer
- Font atlas texture
- Program bindings

To update text resources on the GPU and make them ready for frame rendering, the `Text::Update(...)` method is called for each 
text object in `TypographyApp::Update()`.

```cpp
bool TypographyApp::Update()
{
    if (!UserInterfaceApp::Update())
        return false;

    // Update text block resources
    for(const Ptr<gui::TextItem>& text_ptr : m_texts)
    {
        text_ptr->Update(GetFrameSize());
    }

    return true;
}
```

After updating text resources on the GPU, rendering is straightforward: `UserInterface::Text::Draw(...)` is called for each text 
object with a per-frame render command list and debug group. Font atlas badges are rendered similarly using 
`UserInterface::Badge::Draw(...)`.

```cpp
bool TypographyApp::Render()
{
    if (!UserInterfaceApp::Render())
        return false;

    const TypographyFrame& frame = GetCurrentFrame();

    // Draw text blocks
    META_DEBUG_GROUP_VAR(s_text_debug_group, "Text Blocks Rendering");
    for(const Ptr<gui::TextItem>& text_ptr : m_texts)
    {
        text_ptr->Draw(frame.render_cmd_list, &s_text_debug_group);
    }

    // Draw font atlas badges
    META_DEBUG_GROUP_VAR(s_atlas_debug_group, "Font Atlases Rendering");
    for(const Ptr<gui::Badge>& badge_atlas_ptr : m_font_atlas_badges)
    {
        badge_atlas_ptr->Draw(frame.render_cmd_list, &s_atlas_debug_group);
    }

    RenderOverlay(frame.render_cmd_list);
    frame.render_cmd_list.Commit();

    // Execute command list on render queue and present frame to screen
    GetRenderContext().GetRenderCommandKit().GetQueue().Execute(frame.execute_cmd_list_set);
    GetRenderContext().Present();

    return true;
}
```

## CMake Build Configuration

`TTF` fonts are added to the application embedded resources with `add_methane_embedded_fonts` cmake-function.

```cmake
include(MethaneApplications)

add_methane_application(
    TARGET MethaneTypography
    NAME "Methane Typography"
    DESCRIPTION "Tutorial demonstrating dynamic text rendering and font atlases management with Methane Kit."
    INSTALL_DIR "Apps"
    SOURCES
        TypographyApp.h
        TypographyApp.cpp
        TypographyAppController.h
        TypographyAppController.cpp
)

set(FONTS
    ${RESOURCES_DIR}/Fonts/Roboto/Roboto-Regular.ttf
    ${RESOURCES_DIR}/Fonts/Playball/Playball-Regular.ttf
    ${RESOURCES_DIR}/Fonts/SawarabiMincho/SawarabiMincho-Regular.ttf
)
add_methane_embedded_fonts(MethaneTypography "${RESOURCES_DIR}" "${FONTS}")

target_link_libraries(MethaneTypography
    PRIVATE
        MethaneAppsCommon
)
```

## Continue learning

Continue learning Methane Graphics programming in the next tutorial [CubeMapArray](../06-CubeMapArray),
which is demonstrating cube-map array texturing and sky-box rendering.
