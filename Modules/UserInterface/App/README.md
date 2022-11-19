# User Interface App

[Graphics application](/Modules/Graphics/App) implementation with basic User Interface elements rendered using 
[graphics RHI](/Modules/Graphics/RHI).

## User Interface Application Base Class

### [UserInterface::IApp](Include/Methane/UserInterface/IApp.h)

`UserInterface::IApp` interface contains declaration of `Settings` structure and abstract methods
to get current graphics application settings and methods to modify individual setting values.
The following settings are available:

| IApp Setting             | Type                     | Default Value            | Cmd-Line Option   | Description           |
|--------------------------|--------------------------|--------------------------|-------------------|-----------------------|
| heads_up_display_mode    | HeadsUpDisplayMode       | WindowTitle              | -i,--hud          | HUD displaying mode: Hidden, WindowTitle, UserInterface |
| logo_badge_visible       | bool                     | true                     |                   | Flag to show logo badge |
| logo_badge_color         | Color4F                  | { 1.F, 1.F, 1.F, 0.15F } |                   | Logo badge color |
| text_color               | Color4F                  | { 1.F, 1.F, 1.F, 1.F }   |                   | Default text color in UI panels |
| text_margins             | UnitPoint                | { 20, 20, Units::Dots }  |                   | Text panel margins |
| main_font                | Font::Description        | { "Main",  "RobotoMono-Regular.ttf", 11U } | | Main font parameters |
| hud_settings             | HeadsUpDisplay::Settings | default                  |                   | HUD settings |

### [UserInterface::App](Include/Methane/UserInterface/App.hpp)

`UserInterface::App` application base class is derived from basic graphics application `Graphics::App` class 
and extends it with common UI functionality:
- Controls and shortcuts help rendering in UI panel in left-bottom corner displayed by `F1` key.
- Command-line options help rendering in UI panel in left-bottom corner displayed by `F2` key.
- Optional application parameters rendering in UI panel in right-bottom corner displayed by `F3` key.
- Heads-Up-Display (HUD) rendering in a UI panel in left-top-corner. HUD mode is changed with `F4` key.
- Logo badge rendering in top-right corner.

## User Interface Application Controllers

### [UserInterface::AppController](Include/Methane/UserInterface/AppController.h)

User Interface application controller is derived from the [Graphics application controller](../../Graphics/App/README.md#graphicsappcontrollerincludemethanegraphicsappcontrollerh)
and implements the following additional actions.

| UI Application Action               | Keyboard Shortcut   |
|-------------------------------------|---------------------|
| Switch Heads-Up Display Mode        | F4                  |