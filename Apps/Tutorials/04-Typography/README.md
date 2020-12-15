# Typography Tutorial

| Windows (DirectX 12) | MacOS (Metal) |
| -------------------- | ------------- |
| ![Typography on Windows](Screenshots/TypographyWinDirectX12.jpg) | ![Typography on MacOS](Screenshots/TypographyMacMetal.jpg) |

This tutorial demonstrates animated text rendering with dynamic font atlas updates using Methane UI.
Three colored text blocks are animated with continuous characters typing. Each text block is rendered as a single mesh
displaying character glyphs from the font atlas texture to screen rectanglesq, which are generated on CPU 
using Freetype 2.0 library.

Font atlas texture can be updated dynamically by adding new character glyphs on demand,
as the user types text including any non-Ascii character sets. Text characters layout and mesh generation is done on CPU
using Methane implementation, without using 3rd-party libraries and supports horizontal and vertical text alignment in 
rectangular areas with wrapping by characters and words. Right-to-left and Arabic language characters layout is not supported yet.

## Application and Frame Class Definitions

`TypographyApp` class is declared in header file [TypographyApp.h](TypographyApp.h),
the class is derived from base UI application class `UserInterface::App<TypographyFrame>` which is itself derived from 
`Graphics::App<TypographyFrame>` which is already familiar from the previous tutorials.

```cpp
#pragma once

#include <Methane/Kit.h>
#include <Methane/Data/Receiver.hpp>

namespace Methane::Tutorials
{

namespace gfx = Methane::Graphics;
namespace gui = Methane::UserInterface;

struct TypographyFrame final : gfx::AppFrame
{
    Ptr<gfx::RenderCommandList> render_cmd_list_ptr;
    Ptr<gfx::CommandListSet>    execute_cmd_list_set_ptr;

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
        gui::Text::Layout text_layout                 { gui::Text::Wrap::Word, gui::Text::HorizontalAlignment::Center, gui::Text::VerticalAlignment::Top };
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

    const Settings& GetSettings() const noexcept { return m_settings; }

    void SetTextLayout(const gui::Text::Layout& text_layout);
    void SetForwardTypingDirection(bool is_forward_typing_direction);
    void SetTextUpdateInterval(double text_update_interval_sec);
    void SetIncrementalTextUpdate(bool is_incremental_text_update);

private:
    ...

    Settings            m_settings;
    Ptrs<gui::Font>     m_fonts;
    Ptrs<gui::Text>     m_texts;
    Ptrs<gui::Badge>    m_font_atlas_badges;
    std::vector<size_t> m_displayed_text_lengths;
    double              m_text_update_elapsed_sec = 0.0;
    Timer::TimeDuration m_text_update_duration;
};

} // namespace Methane::Tutorials
```

## Graphics Resources Initialization

## Frame Rendering Cycle

## CMake Build Configuration
