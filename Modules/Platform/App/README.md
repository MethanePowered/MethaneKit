# Platform App

### [Platform::App](Include/Methane/Platform/App.h)

Defines alias type `Platform::App` pointing to one of platform-specific implementations of the `AppBase` abstract class:
- [Platform::AppWin](Include/Methane/Platform/Windows/AppWin.h) - Windows app implementation
- [Platform::AppMac](Include/Methane/Platform/MacOS/AppMac.hh) - MacOS app implementation
- [Platform::AppLin](Include/Methane/Platform/Linux/AppLin.h) - Linux app implementation (stub)

`Platform::App` should be used as a base class for any windowed Methane application.

## Platform Application Base Classes

### [Platform::AppBase](Include/Methane/Platform/AppBase.h)

Base interface and platform-abstraction implementation of the application class with the following functionality:
- Initial application settings setup (see table below)
- Running application with a given command line arguments and parsing platform app settings from cmd arguments
- Live window resizing with frame re-render on resize iterations
- Switching window to full-screen mode
- Processing keyboard and mouse input via `Platform::Input::State`
- Adding [platform application controller](#platform-application-controller) to input state processor
- Setting title in window header
- Showing user alert in message box dialog
- Displaying application controls and command line help
- Shared parallel task execution engine with thread pool [Taskflow::Executor](../../../Externals/TaskFlow)

The following settings are available in `Platform::AppBase::Settings` structure:

| Setting        | Type     | Default Value | Cmd-Line Option  | Description           |
|----------------|----------|---------------|------------------|-----------------------|
| name           | uint32_t | ""            |                  | Application name displayed in window header |
| width          | double   | 0.8           | -w,--width       | Main window width  (if < 1.0 it is a ratio of desktop size; else size in pixels/dots) |
| height         | double   | 0.8           | -x,--height      | Main window height (if < 1.0 it is a ratio of desktop size; else size in pixels/dots) |
| min_width      | uint32_t | 640           |                  | Minimum window width in pixels/dots limited for resizing |
| min_height     | uint32_t | 480           |                  | Minimum window height in pixels/dots limited for resizing |      
| is_full_screen | bool     | false         | -f,--full-screen | Full-screen state of the main window |

## Platform Application Controller

### [Platform::AppController](Include/Methane/Platform/AppController.h)

Platform application controller implements the following actions:

| Action                 | Keyboard Shortcut   |
|------------------------|---------------------|
| Show controls help     | F1                  |
| Show command-line help | F2                  |
| Show parameters        | F3                  |
| Switch full-screen     | LCtrl + F             |
| Close application      | LCtrl/LCmd + Q        |
